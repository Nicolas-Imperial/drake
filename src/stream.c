/*
 Copyright 2015 Nicolas Melot

 This file is part of Drake.

 Drake is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Drake is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Drake. If not, see <http://www.gnu.org/licenses/>.

*/

#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <math.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <execinfo.h>
#include <signal.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <drake/schedule.h>

#if DEBUG
/* get REG_EIP from ucontext.h */
//#define __USE_GNU
#include <ucontext.h>
#endif

#include <pelib/integer.h>

#include <drake/task.h>
#include <drake/link.h>
#include <drake/cross_link.h>
#include <drake/processor.h>
#include <drake/mapping.h>
#include <drake/platform.h>
#include <drake/stream.h>
#include <pelib/monitor.h>

#if 10
#define debug(var) printf("[%s:%s:%d:CORE %zu] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:CORE %zu] %s = \"%p\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:CORE %zu] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:CORE %zu] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

static
void
build_link(mapping_t *mapping, processor_t *proc, task_t *prod, task_t *cons)
{
	int i, j, k;
	link_t *link = NULL;
	cross_link_t *cross_link;
	
	// Find in target task if this link doesn't already exist
	for(k = 0; k < pelib_array_length(link_tp)(prod->succ); k++)
	{
		link = pelib_array_read(link_tp)(prod->succ, k);
		if(link->prod->id == prod->id && link->cons->id == cons->id)
		{
			break;
		}
		else
		{
			link = NULL;
		}
	}

	// If a link between these two tasks does not exist
	if(link == NULL)
	{
		// Create and initialize a new link
		link = (link_t*)drake_platform_private_malloc(sizeof(link_t));
		link->prod = prod;
		link->cons = cons;
		link->buffer = NULL;

		// Add it as source and sink to both current and target tasks
		pelib_array_append(link_tp)(cons->pred, link);
		pelib_array_append(link_tp)(prod->succ, link);

		// If tasks are mapped to different cores
		if(prod->core != cons->core)
		{
			cross_link = (cross_link_t*)drake_platform_private_malloc(sizeof(cross_link_t));
			cross_link->link = link;
			cross_link->read = NULL;
			cross_link->write = NULL;
			cross_link->prod_state = NULL;
			cross_link->total_read = 0;
			cross_link->total_written = 0;
			cross_link->available = 0;

			// Add cross-core link to current task and processor
			pelib_array_append(cross_link_tp)(cons->core->source, cross_link);
			pelib_array_append(cross_link_tp)(cons->source, cross_link);
			// Add cross-core link to target task and its procesor
			pelib_array_append(cross_link_tp)(prod->core->sink, cross_link);
			pelib_array_append(cross_link_tp)(prod->sink, cross_link);
		}
		else
		{
			proc->inner_links++;
		}
	}
}

static array_t(task_tp)*
get_task_consumers(mapping_t *mapping, task_t *task)
{
	size_t i;
	array_t(task_tp) *consumers;
	consumers = pelib_alloc_collection(array_t(task_tp))(mapping->schedule->consumers_in_task[task->id - 1]);
	for(i = 0; i < mapping->schedule->consumers_in_task[task->id - 1]; i++)
	{
		pelib_array_append(task_tp)(consumers, drake_mapping_find_task(mapping, mapping->schedule->consumers_id[task->id - 1][i]));
	}

	return consumers;	
}

static array_t(task_tp)*
get_task_producers(mapping_t *mapping, task_t *task)
{
	array_t(task_tp) *producers;
	size_t i;
	producers = pelib_alloc_collection(array_t(task_tp))(mapping->schedule->producers_in_task[task->id - 1]);
	for(i = 0; i < mapping->schedule->producers_in_task[task->id - 1]; i++)
	{
		size_t task_id = mapping->schedule->producers_id[task->id - 1][i];
		task_t *prod = drake_mapping_find_task(mapping, task_id);
		pelib_array_append(task_tp)(producers, prod);
	}

	return producers;	
}

static
void
build_tree_network(mapping_t* mapping)
{
	int i, j, k;
	task_t *target_task, *current_task;
	link_t *link;
	processor_t *proc;
	cross_link_t *cross_link;

	for(i = 0; i < mapping->processor_count; i++)
	{
		proc = mapping->proc[i];
		proc->inner_links = 0;

		for(j = 0; j < mapping->proc[i]->handled_nodes; j++)
		{
			current_task = mapping->proc[i]->task[j];

			array_t(task_tp) *producers = get_task_producers(mapping, current_task);
			for(k = 0; k < pelib_array_length(task_tp)(producers); k++)
			{
				target_task = pelib_array_read(task_tp)(producers, k);
				if(target_task != NULL)
				{
					build_link(mapping, proc, target_task, current_task);
				}
			}
			pelib_free(array_t(task_tp))(producers);

			array_t(task_tp) *consumers = get_task_consumers(mapping, current_task);
			for(k = 0; k < pelib_array_length(task_tp)(consumers); k++)
			{
				target_task = pelib_array_read(task_tp)(consumers, k);
				if(target_task != NULL)
				{
					build_link(mapping, proc, current_task, target_task);
				}
			}
			pelib_free(array_t(task_tp))(consumers);
		}
	}
}

static int
monitor(task_t *task, cross_link_t *link)
{
	return 0;
}

static
void
feedback_link(task_t *task, cross_link_t *link)
{
	enum task_status new_state;
	size_t size;

	size = link->available - pelib_cfifo_length(int)(link->link->buffer);

	if(size > 0)
	{
#if PRINTF_FEEDBACK
if((printf_enabled & 1) && monitor(task, link)) {
		printf_str(".............................................................");
		printf_str("Feedback");
		printf_str(".............................................................");
		printf_int(*link->read);
		printf_int(link->link->prod->id);
		printf_int(link->link->cons->id);
		printf_int(size);
		printf_int(link->link->buffer->read);
		printf_int(link->link->buffer->write);
		printf_int(link->link->buffer->last_op);
}
#endif
		link->total_read += size;
		*link->read = link->total_read;
		drake_platform_commit(link->read);
		link->available = pelib_cfifo_length(int)(link->link->buffer);
#if PRINTF_FEEDBACK
if((printf_enabled & 1) && monitor(task, link)) {
		printf_int(link->total_read);
		printf_int(link->link->buffer->read);
		printf_int(link->link->buffer->write);
		printf_int(link->link->buffer->last_op);
		printf_int(*link->read);
		printf_str(".............................................................");
}
#endif
	}
}

static
void
push_link(task_t *task, cross_link_t* link)
{
	size_t length = pelib_cfifo_length(int)(link->link->buffer);
	size_t size = length - link->available;
	/*
	printf("[%s:%s:%d][Task %d %s] Number of elements ready to push: %zu\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, length);
	printf("[%s:%s:%d][Task %d %s] Number of elements to be pushed: %zu\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, size);
	printf("[%s:%s:%d][Task %d %s] Available elements in memory: %zu\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, link->available);
	printf("[%s:%s:%d][Task %d %s] Will push %zu elements\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, size);
	printf("[%s:%s:%d][Task %d %s] Writing write counter at address %p\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, link->write);
	printf("[%s:%s:%d][Task %d %s] Writing elements at address %p\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, link->link->buffer->buffer);
	*/

	if(size > 0)
	{
#if PRINTF_PUSH
if((printf_enabled & 1) && monitor(task, link)) {
		printf_str("***********************************************");
		printf_str("Push");
		printf_str("***********************************************");
		printf_int(link->link->prod->id);
		printf_int(link->link->cons->id);
		printf_int(size);
		printf_int(link->link->buffer->read);
		printf_int(link->link->buffer->write);
		printf_int(link->link->buffer->last_op);
		printf_int(size);
}
#endif
		link->total_written += size;
		*link->write = link->total_written;
		drake_platform_commit(link->write);
		link->available = pelib_cfifo_length(int)(link->link->buffer);
#if PRINTF_PUSH
if((printf_enabled & 2) && monitor(task, link)) {
		printf_int(link->total_written);
		printf_int(link->link->buffer->read);
		printf_int(link->link->buffer->write);
		printf_int(link->link->buffer->last_op);
		printf_str("***********************************************");
}
#endif
	}
}

static
void
task_commit(task_t* task)
{
	int i;

	for(i = 0; i < pelib_array_length(cross_link_tp)(task->source); i++)
	{
		feedback_link(task, pelib_array_read(cross_link_tp)(task->source, i));
	}
	for(i = 0; i < pelib_array_length(cross_link_tp)(task->sink); i++)
	{
		push_link(task, pelib_array_read(cross_link_tp)(task->sink, i));
	}
}

static
void
check_input_link(task_t *task, cross_link_t *link)
{
	enum task_status new_state;
	drake_platform_pull(link->write);
	/*
	printf("[%s:%s:%d][Task %d %s] Checking write counter at address %p\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, link->write);
	printf("[%s:%s:%d][Task %d %s] %zu elements written\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, *link->write);
	printf("[%s:%s:%d][Task %d %s] Reading elements at address %p\n", __FILE__, __FUNCTION__, __LINE__, task->id, task->name, link->link->buffer->buffer);
	*/
	size_t write = *link->write - link->total_written;
	size_t actual_write = 0;

	actual_write = pelib_cfifo_fill(int)(link->link->buffer, write);
	link->actual_written = actual_write;

	if(actual_write > 0)
	{
#if PRINTF_CHECK_IN
if((printf_enabled & 4) && monitor(task, link)) {
		printf_str("++++++++++++++++++++++++++++++++++++++++++++++++++++");
		printf_str("Check");
		printf_str("++++++++++++++++++++++++++++++++++++++++++++++++++++");
		printf_int(link->link->prod->id);
		printf_int(link->link->cons->id);
		printf_int(write);
		printf_int(actual_write);
		printf_int(link->link->buffer->read);
		printf_int(link->link->buffer->write);
		printf_int(link->link->buffer->last_op);
}
#endif
		link->available = pelib_cfifo_length(int)(link->link->buffer);
		link->total_written += write;
#if PRINTF_CHECK_IN
if((printf_enabled & 4) && monitor(task, link)) {
		printf_int(write);
		printf_int(link->link->prod->status);
		printf_int(link->total_written);
		printf_int(link->available);
		printf_int(link->link->buffer->read);
		printf_int(link->link->buffer->write);
		printf_int(link->link->buffer->last_op);
		printf_str("++++++++++++++++++++++++++++++++++++++++++++++++++++");
}
#endif
	}

	new_state = *link->prod_state;
	if(new_state != link->link->prod->status)
	{
		link->link->prod->status = new_state;
#if PRINTF_CHECK_IN
		if((printf_enabled & 1) && monitor(task, link)) {
			printf_int(link->link->prod->id);
			printf_int(link->link->cons->id);
			printf_int(link->link->prod->status);
		}
#endif
	}
}

static
void
check_output_link(task_t *task, cross_link_t *link)
{
	enum task_status new_state;
	drake_platform_pull(link->read);
	size_t read = *link->read - link->total_read;
	size_t actual_read = pelib_cfifo_discard(int)(link->link->buffer, read);
	link->actual_read = actual_read;

	if(actual_read > 0)
	{
#if PRINTF_CHECK_OUT
		if((printf_enabled & 8) && monitor(task, link)) {
			printf_str("##############################################################");
			printf_str("Check output_link");
			printf_str("##############################################################");
			printf_int((int)*link->read);
			printf_int((int)read);
			printf_int((int)actual_read);
			printf_int(link->link->prod->id);
			printf_int(link->link->cons->id);
			printf_int(link->link->buffer->read);
			printf_int(link->link->buffer->write);
			printf_int(link->link->buffer->last_op);
		}
#endif
		// Keep track of how much was written before work
		link->available = pelib_cfifo_length(int)(link->link->buffer);
		link->total_read += read;
#if PRINTF_CHECK_OUT
		if((printf_enabled & 8) && monitor(task, link)) {
			printf_int(read);
			printf_int(new_state);
			printf_int(link->total_read);
			printf_int(link->link->buffer->read);
			printf_int(link->link->buffer->write);
			printf_int(link->link->buffer->last_op);
			printf_int((int)*link->read);
			printf_str("##############################################################");
		}
#endif
	}
	else
	{
		/*
		if(read > 0)
		{
			printf_str("Could not receive acknowledgment");
			printf_int(link->link->prod->id);
			printf_int(link->link->cons->id);
			printf_int(read);
			printf_int(actual_read);
			printf_int(link->link->buffer->read);
			printf_int(link->link->buffer->write);
			printf_int(link->link->buffer->last_op);
		}
		*/
	}
}

static
void
task_check(task_t *task)
{
	int i;

	for(i = 0; i < pelib_array_length(cross_link_tp)(task->source); i++)
	{
		check_input_link(task, pelib_array_read(cross_link_tp)(task->source, i));
	}

	for(i = 0; i < pelib_array_length(cross_link_tp)(task->sink); i++)
	{
		check_output_link(task, pelib_array_read(cross_link_tp)(task->sink, i));
	}
}

static
size_t
buffer_size(size_t mpb_size, size_t nb_in, size_t nb_out)
{
	size_t ret = (mpb_size // total allocable MPB size
		- nb_in // As many input buffer as
			* (sizeof(size_t)  // Size of a write
				+ sizeof(enum task_status) // State of a task
			)
		- nb_out // As many output buffers as
			* sizeof(size_t) // Size of a read
		)
	 / nb_in; // Remaining space to be divided by number of input links

	return ret;
}

static
void
printf_mpb_allocation(size_t mpb_size, size_t nb_in, size_t nb_out)
{
	fprintf(stderr, "MPB size: %zu\n", mpb_size);
	fprintf(stderr, "Input links: %zu\n", nb_in);
	fprintf(stderr, "Output links: %zu\n", nb_out);
	fprintf(stderr, "Total allocable memory per input link: %zu\n", buffer_size(mpb_size, nb_in, nb_out < 1));
}

static
void
error_small_mpb(size_t mpb_size, size_t nb_in, size_t nb_out, processor_t *proc)
{
	fprintf(stderr, "Too much cross core links at processor %u\n", proc->id);
	printf_mpb_allocation(mpb_size, nb_in, nb_out);
	abort();
}

static
int
check_mpb_size(size_t mpb_size, size_t nb_in, size_t nb_out, processor_t *proc)
{
	if(buffer_size(mpb_size, nb_in, nb_out) < 1)
	{
		error_small_mpb(mpb_size, nb_in, nb_out, proc);
		return 0;
	}
	else
	{
		return 1;
	}
}

static drake_stack_t*
stack_malloc(size_t size)
{
	drake_stack_t *stack = (drake_stack_t*)drake_platform_private_malloc(sizeof(drake_stack_t));
	//stack->base_ptr = (void*)drake_platform_shared_malloc(size, drake_platform_core_id());
	stack->size = size;
	stack->stack_ptr = (size_t*)drake_platform_private_malloc(sizeof(size_t) * drake_platform_core_size());
	memset(stack->stack_ptr, 0, sizeof(size_t) * drake_platform_core_size());
	return stack;
}

static void*
stack_grow(drake_stack_t *stack, size_t size, int id)
{
	if((size_t)(stack->stack_ptr[id]) + size <= stack->size)
	{
		size_t ptr = stack->stack_ptr[id];
		stack->stack_ptr[id] += size;
		
		return (void*)((size_t)stack->base_ptr + ptr);
	}
	fprintf(stdout, "[%s:%s:%d] Not enough local memory\n", __FILE__, __FUNCTION__, __LINE__);
	abort();
	return NULL;
}

static int
stack_free(drake_stack_t* stack)
{
	free(stack->stack_ptr);
	drake_local_free(stack->base_ptr);
	free(stack);

	return 1;
}

static
void
allocate_buffers(drake_stream_t* stream)
{
	int i, j, k, nb, nb_in, nb_out;
	task_t* task;
	link_t *link;
	cross_link_t *cross_link;
	processor_t *proc;
	mapping_t *mapping = stream->mapping;
	/*
	fprintf(stderr, "[%s:%s:%d] %zX\n", __FILE__, __FUNCTION__, __LINE__, stream->stack);
	fprintf(stderr, "[%s:%s:%d] %zX\n", __FILE__, __FUNCTION__, __LINE__, stream->stack->base_ptr);
	fprintf(stderr, "[%s:%s:%d] %zX\n", __FILE__, __FUNCTION__, __LINE__, stream->stack->stack_ptr);
	drake_stack_t *stack = stream->stack;
	*/
	drake_stack_t *stack = stack_malloc(stream->local_memory_size);

	for(i = 0; i < mapping->processor_count; i++)
	{
		proc = mapping->proc[i];

		for(j = 0; j < proc->handled_nodes; j++)
		{
			task = proc->task[j];

			// Take care of all successor links
			for(k = 0; k < pelib_array_length(link_tp)(task->succ); k++)
			{
				link = pelib_array_read(link_tp)(task->succ, k);
				// If this is not the root task
				if(link->cons != NULL)
				{
					// If the task is mapped to the same core: inner link
					if(link->cons->core->id == proc->id)
					{
						if(link->buffer == NULL)
						{
							size_t capacity = drake_platform_shared_size() / proc->inner_links / sizeof(int);
							link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
					else
					{
						size_t nb_in_succ = pelib_array_length(cross_link_tp)(link->cons->core->source);
						size_t nb_out_succ = pelib_array_length(cross_link_tp)(link->cons->core->sink);
						check_mpb_size(stream->local_memory_size, nb_in_succ, nb_out_succ, proc);

						// If the task is mapped to another core: output link
						if(link->buffer == NULL)
						{
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();
							int core = link->cons->core->id;
							size_t capacity = buffer_size(stream->local_memory_size, nb_in_succ, nb_out_succ) / sizeof(int);
							capacity = capacity - (capacity % drake_platform_shared_align());
							//link->buffer->buffer = (int*)drake_remote_addr(stack_grow(stack, sizeof(int) * capacity, core), core);
							link->buffer->buffer = (int*)drake_platform_shared_malloc(sizeof(int) * capacity, core);
							link->buffer->capacity = capacity;
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
				}
			}

			// Take care of all predecessor links
			for(k = 0; k < pelib_array_length(link_tp)(task->pred); k++)
			{
				link = pelib_array_read(link_tp)(task->pred, k);

				// If this is not the root task
				if(link->prod != NULL)
				{
					// If the task is mapped to the same core: inner link
					if(link->prod->core->id == proc->id)
					{
						if(link->buffer == NULL)
						{
							size_t capacity = drake_platform_shared_size() / proc->inner_links / sizeof(int);
							link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
					else
					{
						nb_in = pelib_array_length(cross_link_tp)(proc->source);
						nb_out = pelib_array_length(cross_link_tp)(proc->sink);
						check_mpb_size(stream->local_memory_size, nb_in, nb_out, proc);
						// If the task is mapped to another core: cross link
						if(link->buffer == NULL)
						{
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();

							int core = task->core->id;
							size_t capacity = buffer_size(stream->local_memory_size, nb_in, nb_out) / sizeof(int);
							capacity = capacity - (capacity % drake_platform_shared_align());
							//link->buffer->buffer = (int*)drake_remote_addr(stack_grow(stack, sizeof(int) * capacity, core), core);
							link->buffer->buffer = (int*)drake_platform_shared_malloc(sizeof(int) * capacity, core);
							link->buffer->capacity = capacity;
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
				}
			}

			// Take care of all output links
			for(k = 0; k < pelib_array_length(cross_link_tp)(task->sink); k++)
			{
				cross_link = pelib_array_read(cross_link_tp)(task->sink, k);

				if(cross_link->read == NULL)
				{
					//cross_link->read = (volatile size_t*)drake_remote_addr(stack_grow(stack, sizeof(size_t), task->core->id), task->core->id);	
					cross_link->read = (volatile size_t*)drake_platform_shared_malloc_mailbox(sizeof(size_t), task->core->id);
					*cross_link->read = 0;
				}
			}

			// Take care of all input links
			for(k = 0; k < pelib_array_length(cross_link_tp)(task->source); k++)
			{
				cross_link = pelib_array_read(cross_link_tp)(task->source, k);

				if(cross_link->prod_state == NULL)
				{
					//cross_link->prod_state = (task_status_t*)drake_remote_addr(stack_grow(stack, sizeof(enum task_status), task->core->id), task->core->id);
					cross_link->prod_state = (task_status_t*)drake_platform_shared_malloc_mailbox(sizeof(enum task_status), task->core->id);
					*cross_link->prod_state = TASK_START;
				}

				if(cross_link->write == NULL)
				{
					//cross_link->write = (volatile size_t*)drake_remote_addr(stack_grow(stack, sizeof(size_t), task->core->id), task->core->id);
					cross_link->write = (volatile size_t*)drake_platform_shared_malloc_mailbox(sizeof(size_t), task->core->id);
					*cross_link->write = 0;
				}
			}
		}
	}
}

#if DEBUG && 0 
void
printf_link(cross_link_t *link)
{
	printf_int(link->link->buffer->read);
	printf_int(link->link->buffer->write);
	printf_int(link->link->buffer->last_op);
	printf_int(pelib_cfifo_length(int)(link->link->buffer));
	printf_int(pelib_cfifo_capacity(int)(link->link->buffer));	

	printf_int(link->available);
	printf_int(link->total_read);
	printf_int(link->total_written);

	printf_int(link->prod_state);
	printf_int(link->actual_read);
	printf_int(link->actual_written);

	pelib_arch_pull(link->read);
	printf_int(*link->read);
	pelib_arch_pull(link->write);
	printf_int(*link->write);
}

task_t *current = NULL;
int signal_counter = 0;
void
bt_sighandler(int sig, siginfo_t *info, void *secret)
{
	void *trace[16];
	char **messages = (char **)NULL, printf_input[3];
	int i, j, trace_size = 0;
	processor_t *proc;
	task_t *task;
	cross_link_t *link;
	ucontext_t *uc = (ucontext_t *)secret;

	printf_str("Waiting for other cores to catch signal");
	drake_platform_barrier(NULL);
	printf_str("Ready to show state");

	if(current != NULL)
	{
		proc = current->core;
		for(i = 0; i < proc->handled_nodes; i++)
		{
			task_t *task = proc->task[i];

			printf_str("++++++++++++++++ Task +++++++++++++++");
			printf_int(task->id);
			printf_int(task->status);

			for(j = 0; j < pelib_array_length(cross_link_tp)(task->source); j++)
			{
				printf_str("=============== Source link ================");
				link = pelib_array_read(cross_link_tp)(task->source, j);
				printf_int(link->link->prod != NULL ? link->link->prod->id : -1);
				printf_link(link);				
			}

			for(j = 0; j < pelib_array_length(cross_link_tp)(task->sink); j++)
			{
				printf_str("================= Sink link ===================");
				link = pelib_array_read(cross_link_tp)(task->sink, j);
				printf_int(link->link->cons != NULL ? link->link->cons->id : -1);
				printf_link(link);				
			}
		}
	}
	else
	{
		printf_str("Current was not assigned yet");
	}

	printf_str("Next output to printf");
	printf_str("0: Nothing");
	printf_str("1: Feedback"); 
	printf_str("2: Push"); 
	printf_str("4: Check in"); 
	printf_str("8: Check out");
	printf_str("16: Get new status");
	printf_str("32: Exit");
	printf_str("Output to printf (add to combine)");
	scanf("%i", &printf_enabled); 

	printf_int(printf_enabled);
	printf_str("Waiting for other cores to continue");
	drake_platform_barrier(NULL);
	printf_str("Resuming computation");

	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
	{
		printf("Got signal %d, faulty address is %p, from %p\n", sig, info->si_addr, uc->uc_mcontext.gregs[REG_EIP]);
	}
	else
	{
		printf("Got signal %d\n", sig);
	}

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	printf("[bt] Execution path:\n");
	for (i = 1; i < trace_size; i++)
	{
		printf("[bt] %s\n", messages[i]);
	}

	if(printf_enabled < 32)
	{
		signal_counter++;
	}
	else
	{
		exit(-1);
	}
}
#endif

static mapping_t*
prepare_mapping(drake_schedule_t *schedule)
{
	size_t i, j;
	mapping_t *mapping;
	processor_t *processor = NULL;
	task_t task;

	size_t num_cores = schedule->core_number;
	mapping = pelib_alloc_collection(mapping_t)(num_cores);
	mapping->schedule = schedule;

	for(j = 1; j <= schedule->core_number; j++)
	{
		size_t tasks_in_core = schedule->tasks_in_core[j - 1];
		processor = pelib_alloc_collection(processor_t)(tasks_in_core);
		processor->id = j - 1;
		size_t producers_in_core = schedule->producers_in_core[j - 1];
		size_t consumers_in_core = schedule->consumers_in_core[j - 1];
		processor->source = pelib_alloc_collection(array_t(cross_link_tp))(producers_in_core);
		processor->sink = pelib_alloc_collection(array_t(cross_link_tp))(consumers_in_core);
		drake_mapping_insert_processor(mapping, processor);

		for(i = 1; i <= schedule->tasks_in_core[j - 1]; i++)
		{
			task.id = schedule->schedule[j - 1][i - 1].id;
			task.name = schedule->task_name[task.id - 1];
			task.frequency = schedule->schedule[j - 1][i - 1].frequency;
			size_t producers_in_task = schedule->producers_in_task[task.id - 1];
			size_t consumers_in_task = schedule->consumers_in_task[task.id - 1];
			size_t remote_producers_in_task = schedule->remote_producers_in_task[task.id - 1];
			size_t remote_consumers_in_task = schedule->remote_consumers_in_task[task.id - 1];
			task.pred = pelib_alloc_collection(array_t(link_tp))(producers_in_task);
			task.succ = pelib_alloc_collection(array_t(link_tp))(consumers_in_task);
			task.source = pelib_alloc_collection(array_t(cross_link_tp))(remote_producers_in_task);
			task.sink = pelib_alloc_collection(array_t(cross_link_tp))(remote_consumers_in_task);

			task.status = TASK_START;

			drake_mapping_insert_task(mapping, j - 1, &task);
		}
	}

	return mapping;
}

drake_stream_t
drake_stream_create_explicit(void (*schedule_init)(drake_schedule_t*), void (*schedule_destroy)(drake_schedule_t*), void* (*task_function)(size_t id, task_status_t status))
{
	int k;
	char* outputfile;
	array_t(int) *array = NULL;
	drake_stream_t stream;

	// Initialize functions
	stream.schedule_destroy = schedule_destroy;
	mapping_t *mapping;
	int i, j, max_nodes;
	int rcce_id;

	processor_t *proc;
	task_t *task;
	link_t* link;

	unsigned long long int start, stop, begin, end;
	schedule_init(&stream.schedule);
	drake_schedule_t *schedule = &stream.schedule;
	drake_platform_barrier(NULL);
	mapping = prepare_mapping(schedule);

	// Build task network based on tasks' id
	build_tree_network(mapping);
	proc = mapping->proc[drake_mapping_find_processor_index(mapping, drake_platform_core_id())];

	// Initialize task's pointer
	max_nodes = proc->handled_nodes;
	for(i = 0; i < max_nodes; i++)
	{
		task_t *task = proc->task[i];
		task->status = TASK_START;
		task->init = (int (*)(task_t*, void*))task_function(task->id, TASK_INIT);
		task->start = (int (*)(task_t*))task_function(task->id, TASK_START);
		task->run = (int (*)(task_t*))task_function(task->id, TASK_RUN);
		task->destroy = (int (*)(task_t*))task_function(task->id, TASK_DESTROY);
		task->kill = (int (*)(task_t*))task_function(task->id, TASK_KILLED);
	}

	stream.mapping = mapping;
	stream.proc = proc;
	stream.local_memory_size = drake_platform_shared_size();
	stream.stage_start_time = drake_platform_time_alloc();
	stream.stage_stop_time = drake_platform_time_alloc();
	stream.stage_sleep_time = drake_platform_time_alloc();
	stream.stage_time = drake_platform_time_alloc();
	drake_platform_time_init(stream.stage_time, schedule->stage_time);
	stream.zero = drake_platform_time_alloc();
	drake_platform_time_init(stream.zero, 0);

	return stream;
}

int
drake_stream_init(drake_stream_t *stream, void *aux)
{
	int success = 1;
	size_t i;
	size_t max_nodes = stream->proc->handled_nodes;

	allocate_buffers(stream);
	for(i = 0; i < max_nodes; i++)
	{
		task_t *task = stream->proc->task[i];
		int run = task->init(task, aux);
		success = success && run;		
	}
	drake_platform_barrier(NULL);

	return success;
}

int
drake_stream_destroy(drake_stream_t* stream)
{
	// Run the destroy method for each task
	task_t *task;
	int i;
	int success;
	mapping_t *mapping = stream->mapping;
	processor_t *proc = stream->proc;
	for(i = 0; i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		task->status = TASK_DESTROY;
		success = success && task->destroy(task);
	}

	// Free the stream data structures
	stream->schedule_destroy(&stream->schedule);
	free(stream->stage_start_time);
	free(stream->stage_stop_time);
	free(stream->stage_sleep_time);
	free(stream->stage_time);
	free(stream->zero);

	return success;
}

int
drake_stream_run(drake_stream_t* stream)
{
	unsigned long long int start, stop;
	size_t i, j;
	task_t *task;
	mapping_t *mapping = stream->mapping;
	processor_t *proc = stream->proc;
	int active_tasks = proc->handled_nodes;
	unsigned long long int begin, end, obegin, oend;

	// Make sure everyone starts at the same time
	time_t timeref = time(NULL);
	int done = 0;

	/* Phase 1, real */
	while(active_tasks > 0)
	{
		// Capture the stage starting time
		drake_platform_time_get(stream->stage_start_time);
		
		for(i = 0; i < proc->handled_nodes; i++)
		{
			task = proc->task[i];
			switch(task->status)
			{
				// Checks input and proceeds to start when first input come
				case TASK_START:
					done = task->start(task);
					task_check(task);
					if(task->status == TASK_START && done)
					{
						task->status = TASK_RUN;
					}
					break;
				case TASK_RUN:
					// Check
					task_check(task);

					// Switch frequency
					if(drake_platform_get_frequency() != task->frequency)
					{
						drake_platform_set_voltage_frequency(task->frequency);
					}

					// Work
					done = task->run(task);

					// Commit
					task_commit(task);

					if(task->status == TASK_RUN && done)
					{
						
						task->status = TASK_KILLED;
					}

				// Stop stopwatch and decrement active tasks
				case TASK_KILLED:
					if(task->status == TASK_KILLED)
					{
						task->kill(task);
						// Send successor tasks the task killed information
						for(j = 0; j < pelib_array_length(cross_link_tp)(task->sink); j++)
						{
							cross_link_t *link = pelib_array_read(cross_link_tp)(task->sink, j);
							*link->prod_state = task->status;
							drake_platform_commit(link->prod_state);
						}
						task->status = TASK_ZOMBIE;
						active_tasks--;
					}
				break;

				// Loop until all other tasks are also done
				case TASK_ZOMBIE:
					if(task->status == TASK_ZOMBIE)
					{
						task->status = TASK_ZOMBIE;
					}
				break;

				// Should not happen. If it does happen, let the scheduler decide what to do
				case TASK_INVALID:
					task->status = TASK_ZOMBIE;
				break;

				default:
					task->status = TASK_INVALID;
				break;
			}
		}

		// Pause until the stage time elapses, if any
		if(drake_platform_time_greater(stream->stage_time, stream->zero))
		{
			drake_platform_time_get(stream->stage_stop_time);
			drake_platform_time_substract(stream->stage_sleep_time, stream->stage_stop_time, stream->stage_start_time);
			drake_platform_time_substract(stream->stage_sleep_time, stream->stage_time, stream->stage_sleep_time);
			drake_platform_sleep(stream->stage_sleep_time);
		}
	}

	return 0;
}
