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
#include <execinfo.h>

#include <pelib/integer.h>

#include <drake/task.h>
#include <drake/link.h>
#include <drake/cross_link.h>
#include <drake/processor.h>
#include <drake/mapping.h>
#include <drake/platform.h>
#include <drake/stream.h>
#include <pelib/monitor.h>

// Debuggin options
#if 1
#define debug(var) printf("[%s:%s:%d:P%zu] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:P%zu] %s = \"%p\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:P%zu] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_uint(var) printf("[%s:%s:%d:P%zu] %s = \"%u\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_luint(var) printf("[%s:%s:%d:P%zu] %s = \"%lu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:P%zu] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

#define link_name_to_int(name) strcmp((name), "left") == 0 || strcmp((name), "output") == 0 ? 0 : 1

static
int
tasks_mapped_same_cores(task_tp t1, task_tp t2)
{
	if(t1->width != t2->width)
	{
		return 0;
	}
	else
	{
		size_t i;
		for(i = 0; i < t1->width; i++)
		{
			if(t1->core[i]->id != t2->core[i]->id)
			{
				return 0;
			}
		}
		return 1;
	}
}

static
void
build_link(mapping_t *mapping, processor_t *proc, task_t *prod, task_t *cons, string prod_name, string cons_name)
{
	size_t i, j;
	link_t *link = NULL;
	cross_link_t *cross_link;
	
	// Find in target task if this link doesn't already exist
	map_iterator_t(string, link_tp) *kk;
	for(kk = pelib_map_begin(string, link_tp)(prod->succ); kk != pelib_map_end(string, link_tp)(prod->succ); kk = pelib_map_next(string, link_tp)(kk))
	{
		pair_t(string, link_tp) string_link_pair = pelib_map_read(string, link_tp)(kk);
		link = string_link_pair.value;
		if(
			link->prod->id == prod->id
			&& link->cons->id == cons->id
			&& pelib_compare(string)(string_link_pair.key, prod_name) == 0 // Must also check if the link we found is the proper link
		)
		{
			break;
		}
		else
		{
			link = NULL;
		}
	}

	// If a link between these two tasks does not exist but *should be* according to schedule
	// Let us consider this link should not be created unless proven otherwise
	// First let's see if any of the producer task's consumer has an input link with this name
	int should_be_created = 0;
	for(i = 0; i < mapping->schedule->consumers_in_task[prod->id - 1]; i++)
	{
		// If the producer task has an output link the consumer task sees as this consumer name
		if(strcmp(mapping->schedule->consumers_name[prod->id - 1][i], cons_name) == 0)
		{
			// Alright, there is such a consumer. Let's see if this consumer has the
			// expected task name and its input link points to this producer with this
			// output link name
			if(strcmp(mapping->schedule->output_name[prod->id - 1][i], prod_name) == 0)
			{
				should_be_created = 1;
				break;
			}
		}
	}

	// Create the link only if the link does not already exist AND if the link is valid according to schedule.
	if(
		link == NULL
		&& should_be_created != 0
	)
	{
		// Create and initialize a new link
		link = (link_t*)drake_platform_private_malloc(sizeof(link_t));
		link->prod = prod;
		link->cons = cons;
		link->buffer = NULL;

		//printf("[%s:%s:%d] Adding link from task \"%s\" with output port name \"%s\" to task \"%s\" with input port name \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, prod->name, prod_name, cons->name, cons_name);

		// Add it as source and sink to both current and target tasks
		pair_t(string, link_tp) link_prod_pair, link_cons_pair;
		pelib_alloc_buffer(string)(&link_prod_pair.key, (strlen(prod_name) + 1) * sizeof(char));
		pelib_alloc_buffer(string)(&link_cons_pair.key, (strlen(cons_name) + 1) * sizeof(char));
		pelib_copy(string)(prod_name, &link_prod_pair.key);
		pelib_copy(string)(cons_name, &link_cons_pair.key);
		pelib_copy(link_tp)(link, &link_prod_pair.value);
		pelib_copy(link_tp)(link, &link_cons_pair.value);
		pelib_map_insert(string, link_tp)(prod->succ, link_prod_pair);
		pelib_map_insert(string, link_tp)(cons->pred, link_cons_pair);

		// If tasks are mapped to different sets of cores
		size_t i;
		for(i = 0; i < (prod->width < cons->width ? prod->width : cons->width); i++)
		{
			if(prod->core[i] != cons->core[i])
			{
				break;
			}
		}

		// Then create a cross link between these tasks
		if(prod->width != cons->width || !tasks_mapped_same_cores(prod, cons))
		//if(prod->width != cons->width || i < (prod->width < cons->width ? prod->width : cons->width))
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
			size_t min = pelib_array_length(cross_link_tp)(cons->core[0]->source);
			size_t min_index = 0;
			for(i = 1; i < cons->width; i++)
			{
				if(min > pelib_array_length(cross_link_tp)(cons->core[i]->source))
				{
					min = pelib_array_length(cross_link_tp)(cons->core[i]->source);
					min_index = i;
				}
			}
			pelib_array_append(cross_link_tp)(cons->core[min_index]->source, cross_link);
			pelib_array_append(cross_link_tp)(cons->source, cross_link);

			// Add cross-core link to target task and its processor
			min = pelib_array_length(cross_link_tp)(prod->core[0]->source);
			min_index = 0;
			for(i = 1; i < prod->width; i++)
			{
				if(min > pelib_array_length(cross_link_tp)(prod->core[i]->source))
				{
					min = pelib_array_length(cross_link_tp)(prod->core[i]->source);
					min_index = i;
				}
			}
			pelib_array_append(cross_link_tp)(prod->core[min_index]->sink, cross_link);
			pelib_array_append(cross_link_tp)(prod->sink, cross_link);

		}
		else
		{
			proc->inner_links++;
		}
	}
}

static map_t(pair_t(string, string), task_tp)*
get_task_consumers(mapping_t *mapping, task_t *task)
{
	size_t i;
	map_t(pair_t(string, string), task_tp) *consumers;
	consumers = pelib_alloc(map_t(pair_t(string, string), task_tp))();
	pelib_init(map_t(pair_t(string, string), task_tp))(consumers);
	for(i = 0; i < mapping->schedule->consumers_in_task[task->id - 1]; i++)
	{
		pair_t(pair_t(string, string), task_tp) pair;
		string name = mapping->schedule->consumers_name[task->id - 1][i];
		string output = mapping->schedule->output_name[task->id - 1][i];
		pelib_alloc_buffer(string)(&pair.key.value, strlen(name) + 1);
		pelib_copy(string)(name, &pair.key.value);
		pelib_alloc_buffer(string)(&pair.key.key, strlen(output) + 1);
		pelib_copy(string)(output, &pair.key.key);
		pair.value = drake_mapping_find_task(mapping, mapping->schedule->consumers_id[task->id - 1][i]);
		pelib_map_insert(pair_t(string, string), task_tp)(consumers, pair);
	}

	return consumers;	
}

static map_t(pair_t(string, string), task_tp)*
get_task_producers(mapping_t *mapping, task_t *task)
{
	size_t i;
	map_t(pair_t(string, string), task_tp) *producers;
	producers = pelib_alloc(map_t(pair_t(string, string), task_tp))();
	pelib_init(map_t(pair_t(string, string), task_tp))(producers);
	for(i = 0; i < mapping->schedule->producers_in_task[task->id - 1]; i++)
	{
		pair_t(pair_t(string, string), task_tp) pair;
		string name = mapping->schedule->producers_name[task->id - 1][i];
		string input = mapping->schedule->input_name[task->id - 1][i];
		pelib_alloc_buffer(string)(&pair.key.key, strlen(name) + 1);
		pelib_copy(string)(name, &pair.key.key);
		pelib_alloc_buffer(string)(&pair.key.value, strlen(input) + 1);
		pelib_copy(string)(input, &pair.key.value);
		pair.value = drake_mapping_find_task(mapping, mapping->schedule->producers_id[task->id - 1][i]);
		pelib_map_insert(pair_t(string, string), task_tp)(producers, pair);
	}

	return producers;	
}

static
void
build_tree_network(mapping_t* mapping)
{
	int i, j;
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

			map_t(pair_t(string, string), task_tp) *producers = get_task_producers(mapping, current_task);
			map_iterator_t(pair_t(string, string), task_tp)* kk;
			for(kk = pelib_map_begin(pair_t(string, string), task_tp)(producers); kk != pelib_map_end(pair_t(string, string), task_tp)(producers); kk = pelib_map_next(pair_t(string, string), task_tp)(kk))
			{
				target_task = pelib_map_read(pair_t(string, string), task_tp)(kk).value;
				string prod_name = pelib_map_read(pair_t(string, string), task_tp)(kk).key.key;
				string cons_name = pelib_map_read(pair_t(string, string), task_tp)(kk).key.value;

				if(target_task != NULL)
				{
					build_link(mapping, proc, target_task, current_task, prod_name, cons_name);

					size_t l;
					for(l = 0; l < current_task->width; l++)
					{
						int index = drake_mapping_find_processor_index(mapping, current_task->core[l]->id);
						task_t *t = mapping->proc[index]->task[drake_processor_find_task(mapping->proc[index], current_task->id)];
						*t = *current_task;
					}

					for(l = 0; l < target_task->width; l++)
					{
						int index = drake_mapping_find_processor_index(mapping, target_task->core[l]->id);
						task_t *t = mapping->proc[index]->task[drake_processor_find_task(mapping->proc[index], target_task->id)];
						*t = *target_task;
					}
				}
			}
			//pelib_destroy(map_t(string, task_tp))(*producers);
			pelib_free(map_t(pair_t(string, string), task_tp))(producers);

			map_t(pair_t(string, string), task_tp) *consumers = get_task_consumers(mapping, current_task);
			for(kk = pelib_map_begin(pair_t(string, string), task_tp)(consumers); kk != pelib_map_end(pair_t(string, string), task_tp)(consumers); kk = pelib_map_next(pair_t(string, string), task_tp)(kk))
			{
				target_task = pelib_map_read(pair_t(string, string), task_tp)(kk).value;

				if(target_task != NULL)
				{
					string prod_name = pelib_map_read(pair_t(string, string), task_tp)(kk).key.key;
					string cons_name = pelib_map_read(pair_t(string, string), task_tp)(kk).key.value;
					build_link(mapping, proc, current_task, target_task, prod_name, cons_name);

					size_t l;
					for(l = 0; l < current_task->width; l++)
					{
						int index = drake_mapping_find_processor_index(mapping, current_task->core[l]->id);
						task_t *t = mapping->proc[index]->task[drake_processor_find_task(mapping->proc[index], current_task->id)];
						*t = *current_task;
					}
					for(l = 0; l < target_task->width; l++)
					{
						int index = drake_mapping_find_processor_index(mapping, target_task->core[l]->id);
						task_t *t = mapping->proc[index]->task[drake_processor_find_task(mapping->proc[index], target_task->id)];
						*t = *target_task;
					}
				}
			}
			//pelib_destroy(map_t(string, task_tp))(*consumers);
			pelib_free(map_t(pair_t(string, string), task_tp))(consumers);
		}
	}
}

#if PRINTF_FEEDBACK || PRINTF_PUSH || PRINTF_CHECK_IN || PRINTF_CHECK_OUT
static int
monitor(task_t *task, cross_link_t *link)
{
	return 0;
}
int printf_enabled = -1;
#endif

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
}
#endif
		link->total_read += size;
		*link->read = link->total_read;
		drake_platform_commit(link->read);
		link->available = pelib_cfifo_length(int)(link->link->buffer);
#if PRINTF_FEEDBACK
if((printf_enabled & 1) && monitor(task, link)) {
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
}
#endif
		link->total_written += size;
		*link->write = link->total_written;
		drake_platform_commit(link->write);
		link->available = pelib_cfifo_length(int)(link->link->buffer);
#if PRINTF_PUSH
if((printf_enabled & 2) && monitor(task, link)) {
}
#endif
	}

	// Also send the latest task status
	*link->prod_state = task->status;
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
}
#endif
		link->available = pelib_cfifo_length(int)(link->link->buffer);
		link->total_written += write;
#if PRINTF_CHECK_IN
if((printf_enabled & 4) && monitor(task, link)) {
}
#endif
	}

	new_state = *link->prod_state;
	if(new_state != link->link->prod->status)
	{
		link->link->prod->status = new_state;
#if PRINTF_CHECK_IN
		if((printf_enabled & 1) && monitor(task, link)) {
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
		}
#endif
		// Keep track of how much was written before work
		link->available = pelib_cfifo_length(int)(link->link->buffer);
		link->total_read += read;
#if PRINTF_CHECK_OUT
		if((printf_enabled & 8) && monitor(task, link)) {
		}
#endif
	}
	else
	{
		/*
		if(read > 0)
		{
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
#if 0
	size_t ret = (nb_in > 0) ? (mpb_size // total allocable MPB size
		- nb_in // As many input buffer as
			* (sizeof(size_t)  // Size of a write
				+ sizeof(enum task_status) // State of a task
			)
		- nb_out // As many output buffers as
			* sizeof(size_t) // Size of a read
		)
	/ nb_in : mpb_size; // Remaining space to be divided by number of input links
#endif

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
	*/

	for(i = 0; i < mapping->processor_count; i++)
	{
		proc = mapping->proc[i];

		for(j = 0; j < proc->handled_nodes; j++)
		{
			task = proc->task[j];

			// Take care of all successor links
			map_iterator_t(string, link_tp)* kk;
			for(kk = pelib_map_begin(string, link_tp)(task->succ); kk != pelib_map_end(string, link_tp)(task->succ); kk = pelib_map_next(string, link_tp)(kk))
			{
				link = pelib_map_read(string, link_tp)(kk).value;
				// If this is not the root task
				if(link->cons != NULL)
				{
					// If the task is mapped to the same core: inner link
					if(tasks_mapped_same_cores(task, link->cons))
					{
						if(link->buffer == NULL)
						{
/*
							debug("Allocate inner buffer");
							debug(task->name);
							debug(link->cons->name);
							debug_size_t(drake_platform_shared_size());
							debug_int(proc->inner_links);
							debug("#########################");
*/
							size_t capacity = drake_platform_shared_size() / proc->inner_links / sizeof(int);
							link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
					else
					{
						size_t l;
						for(l = 0; l < link->cons->width; l++)
						{
							size_t m;
							for(m = 0; m < pelib_array_length(cross_link_tp)(link->cons->core[l]->source); m++)
							{
								if(pelib_array_read(cross_link_tp)(link->cons->core[l]->source, m)->link->prod->id == task->id)
								{
									break;
								}
							}

							if(m < pelib_array_length(cross_link_tp)(link->cons->core[l]->source))
							{
								break;
							}
						}
						if(l == link->cons->width)
						{
							fprintf(stderr, "[%s:%d:P%zu] Something has gone terribly wrong here\n", __FILE__, __LINE__, drake_platform_core_id());
							abort();
						}
						size_t nb_in_succ = pelib_array_length(cross_link_tp)(link->cons->core[l]->source);
						size_t nb_out_succ = pelib_array_length(cross_link_tp)(link->cons->core[l]->sink);
						check_mpb_size(stream->local_memory_size, nb_in_succ, nb_out_succ, proc);

						// If the task is mapped to another core: output link
						if(link->buffer == NULL)
						{
/*
							debug("Allocate inter-core buffer");
							debug(task->name);
							debug(link->cons->name);
							debug_size_t(drake_platform_shared_size());
							debug_int(proc->inner_links);
							debug("#########################");
*/
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();
							int core = link->cons->core[l]->id;
							size_t capacity = buffer_size(stream->local_memory_size, nb_in_succ, nb_out_succ) / sizeof(int);
							capacity = capacity - (capacity % drake_platform_shared_align());
							link->buffer->buffer = (int*)drake_platform_shared_malloc(sizeof(int) * capacity, core);
							link->buffer->capacity = capacity;
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
				}
			}

			// Take care of all predecessor links
			for(kk = pelib_map_begin(string, link_tp)(task->pred); kk != pelib_map_end(string, link_tp)(task->pred); kk = pelib_map_next(string, link_tp)(kk))
			{
				link = pelib_map_read(string, link_tp)(kk).value;

				// If this is not the root task
				if(link->prod != NULL)
				{
					// If the task is mapped to the same core: inner link
					if(tasks_mapped_same_cores(task, link->prod))
					{
						if(link->buffer == NULL)
						{
/*
							debug("Allocate inner buffer");
							debug(link->prod->name);
							debug(task->name);
							debug("#########################");
*/
							size_t capacity = drake_platform_shared_size() / proc->inner_links / sizeof(int);
							link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
					else
					{
						size_t l;
						for(l = 0; l < link->cons->width; l++)
						{
							size_t m;
							for(m = 0; m < pelib_array_length(cross_link_tp)(link->cons->core[l]->source); m++)
							{
								if(pelib_array_read(cross_link_tp)(link->cons->core[l]->source, m)->link->prod->id == link->prod->id)
								{
									break;
								}
							}

							if(m < pelib_array_length(cross_link_tp)(link->cons->core[l]->source))
							{
								break;
							}
						}

						nb_in = pelib_array_length(cross_link_tp)(proc->source);
						nb_out = pelib_array_length(cross_link_tp)(proc->sink);
						check_mpb_size(stream->local_memory_size, nb_in, nb_out, proc);
						// If the task is mapped to another core: cross link
						if(link->buffer == NULL)
						{
/*
							debug("Allocate inter buffer");
							debug(link->prod->name);
							debug(task->name);
							debug("#########################");
*/
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();

							//int core = task->core[l]->id;
							int core = link->cons->core[l]->id;
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
					size_t l;
					for(l = 0; l < cross_link->link->cons->width; l++)
					{
						size_t m;
						for(m = 0; m < pelib_array_length(cross_link_tp)(cross_link->link->cons->core[l]->source); m++)
						{
							if(pelib_array_read(cross_link_tp)(cross_link->link->cons->core[l]->source, m)->link->prod->id == cross_link->link->prod->id)
							{
								break;
							}
						}

						if(m < pelib_array_length(cross_link_tp)(cross_link->link->cons->core[l]->source))
						{
							break;
						}
					}
					if(l == cross_link->link->cons->width)
					{
						fprintf(stderr, "[%s:%d:P%zu] Something has gone terribly wrong here\n", __FILE__, __LINE__, drake_platform_core_id());
						abort();
					}
					cross_link->read = (volatile size_t*)drake_platform_shared_malloc_mailbox(sizeof(size_t), cross_link->link->cons->core[l]->id);
					
					*cross_link->read = 0;
				}
			}

			// Take care of all input links
			for(k = 0; k < pelib_array_length(cross_link_tp)(task->source); k++)
			{
				cross_link = pelib_array_read(cross_link_tp)(task->source, k);
				size_t l;
				for(l = 0; l < cross_link->link->prod->width; l++)
				{
					size_t m;
					for(m = 0; m < pelib_array_length(cross_link_tp)(link->prod->core[l]->sink); m++)
					{
						if(pelib_array_read(cross_link_tp)(cross_link->link->prod->core[l]->sink, m)->link->prod->id == cross_link->link->prod->id)
						{
							break;
						}
					}

					if(m < pelib_array_length(cross_link_tp)(cross_link->link->prod->core[l]->sink))
					{
						break;
					}
				}
				if(l == cross_link->link->cons->width)
				{
					fprintf(stderr, "[%s:%d:P%zu] Something has gone terribly wrong here\n", __FILE__, __LINE__, drake_platform_core_id());
					abort();
				}


				if(cross_link->prod_state == NULL)
				{
					cross_link->prod_state = (task_status_t*)drake_platform_shared_malloc_mailbox(sizeof(enum task_status), cross_link->link->cons->core[l]->id);
					*cross_link->prod_state = TASK_START;
				}

				if(cross_link->write == NULL)
				{
					cross_link->write = (volatile size_t*)drake_platform_shared_malloc_mailbox(sizeof(size_t), cross_link->link->cons->core[l]->id);
					*cross_link->write = 0;
				}
			}
		}
	}
}

#if MONITOR_EXCEPTIONS

task_t *current = NULL;
struct sigaction oldact[5];
int signal_counter = 0;
void
bt_sighandler(int sig, siginfo_t *info, void *secret)
{
	// Restores normal signal catching
        sigaction(SIGSEGV, &oldact[0], NULL);
        sigaction(SIGUSR1, &oldact[1], NULL);
        sigaction(SIGINT, &oldact[2], NULL);
        sigaction(SIGFPE, &oldact[3], NULL);
        sigaction(SIGTERM, &oldact[4], NULL);

	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace (array, 10);
	strings = backtrace_symbols (array, size);

	if(drake_platform_core_id() == 0)
	{
	printf("[%s:%s:%d:P%zu] Caught signal %d\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), sig);
	switch(sig)
	{
		case SIGFPE:
		{
			printf("[%s:%s:%d:P%zu] Caught floating-point exception (SIGFPE)\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id());
		}
		break;
		case SIGSEGV:
		{
			printf("[%s:%s:%d:P%zu] Caught segmentation fault exception (SIGSEGV)\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id());
		}
		break;
		default:
		{
			printf("[%s:%s:%d:P%zu] Caught unknown signal: %d\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), sig);
		}
		break;
	}
	
	printf("[%s:%s:%d:P%zu] Obtained %zd stack frames.\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), size);
	printf("[%s:%s:%d:P%zu] ", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id());

	for (i = 0; i < size; i++)
	{
		printf("%s ", strings[i]);
	}
	printf("\n");
	}

	abort();
}
#endif

static mapping_t*
prepare_mapping(drake_schedule_t *schedule)
{
	size_t i, j;
	mapping_t *mapping;
	processor_t *processor = NULL;

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
	}

	size_t task_counter = 0;
	task_t *tasks = malloc(sizeof(task_t) * schedule->task_number);	
	for(j = 1; j <= schedule->core_number; j++)
	{
		for(i = 1; i <= schedule->tasks_in_core[j - 1]; i++)
		{
			// Check if this that has been already mapped or not
			size_t k;
			for(k = 0; k < task_counter; k++)
			{
				if(tasks[k].id == schedule->schedule[j - 1][i - 1].id)
				{
					break;
				}
			}
			if(k == task_counter)
			{
				// This is a new task
				task_t task;
				task.id = schedule->schedule[j - 1][i - 1].id;
				task.name = schedule->task_name[task.id - 1];
        			task.core = malloc(sizeof(processor_t*));
			        task.core[0] = mapping->proc[j - 1];
			        task.width = 1;
				tasks[task_counter] = task;
				task_counter++;

			}
			else
			{
				// We update an existing task
				processor_t **list = tasks[k].core;
				tasks[k].core = malloc(sizeof(processor_t*) * (tasks[k].width + 1));
				memcpy(tasks[k].core, list, sizeof(processor_t*) * tasks[k].width);
				tasks[k].core[tasks[k].width] = mapping->proc[j - 1];
				tasks[k].width++;
				free(list);
			}
		}
	}
	for(j = 1; j <= schedule->core_number; j++)
	{
		for(i = 1; i <= schedule->tasks_in_core[j - 1]; i++)
		{
			size_t k;
			task_t task;
			for(k = 0; k < task_counter; k++)
			{
				if(tasks[k].id == schedule->schedule[j - 1][i - 1].id)
				{
					task = tasks[k];
					break;
				}
			}
			if(k == task_counter)
			{
				fprintf(stderr, "Catastrophic error at %s:%d: trying to create a task that is not in schedule. This is non-sense, aborting.\n", __FILE__, __LINE__);
				abort();
			}
			task.name = schedule->task_name[task.id - 1];
			task.workload = schedule->task_workload[task.id - 1];
			task.frequency = schedule->schedule[j - 1][i - 1].frequency;
			size_t producers_in_task = schedule->producers_in_task[task.id - 1];
			size_t consumers_in_task = schedule->consumers_in_task[task.id - 1];
			size_t remote_producers_in_task = schedule->remote_producers_in_task[task.id - 1];
			size_t remote_consumers_in_task = schedule->remote_consumers_in_task[task.id - 1];
			task.pred = pelib_alloc(map_t(string, link_tp))();
			task.succ = pelib_alloc(map_t(string, link_tp))();
			pelib_init(map_t(string, link_tp))(task.pred);
			pelib_init(map_t(string, link_tp))(task.succ);
			task.source = pelib_alloc_collection(array_t(cross_link_tp))(remote_producers_in_task);
			task.sink = pelib_alloc_collection(array_t(cross_link_tp))(remote_consumers_in_task);

			task.status = TASK_START;
			drake_mapping_insert_task(mapping, j - 1, &task);
		}
	}
	free(tasks);

	return mapping;
}

drake_stream_t
drake_stream_create_explicit(void (*schedule_init)(drake_schedule_t*), void (*schedule_destroy)(drake_schedule_t*), void* (*task_function)(size_t id, task_status_t status), drake_platform_t pt)
{
	drake_stream_t stream;
	int k;
	char* outputfile;
	array_t(int) *array = NULL;

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
	if(stream.schedule.core_number != drake_platform_core_size())
	{
		fprintf(stderr, "Application compiled for different number of cores (%zu) than available on this platform (%zu). Please recompile drake application with a correct platform description\n", stream.schedule.core_number, drake_platform_core_size());
		abort();
	}
	drake_schedule_t *schedule = &stream.schedule;
	drake_platform_barrier(NULL);
	mapping = prepare_mapping(schedule);

	// Build task network based on tasks' id
	build_tree_network(mapping);
	size_t stuff1 = drake_platform_core_id();
	int stuff2 = drake_mapping_find_processor_index(mapping, stuff1);
	proc = mapping->proc[stuff2];

	// Initialize task's pointer
	max_nodes = (proc == NULL ? 0 : proc->handled_nodes);
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

	stream.platform = pt;

	return stream;
}

int
drake_stream_init(drake_stream_t *stream, void *aux)
{
	int success = 1;
	size_t i;

	if(stream->proc != NULL && stream->proc->handled_nodes > 0)
	{
		size_t max_nodes = stream->proc->handled_nodes;
		allocate_buffers(stream);
		for(i = 0; i < max_nodes; i++)
		{
			task_t *task = stream->proc->task[i];
			int run = task->init(task, aux);
			//int run = 1;
			success = success && run;		
		}
		drake_platform_barrier(NULL);
	}
	else
	{
		// Deactivate core is no task is to be run
		drake_platform_core_disable(stream->platform, drake_platform_core_id());
		drake_platform_barrier(NULL);
	}

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
	for(i = 0; proc != NULL && i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		task->status = TASK_DESTROY;
		// /!\ Do not invert the operand of the boolean expression below
		// Or Drake will not run a destroy method of a task if a previous
		// task did not return 1 when destroyed.
		success = task->destroy(task) && success;
	}

	// Free the stream data structures
	stream->schedule_destroy(&stream->schedule);
	free(stream->stage_start_time);
	free(stream->stage_stop_time);
	free(stream->stage_sleep_time);
	free(stream->stage_time);
	free(stream->zero);

	// TODO: deallocate buffers, if core had any task to run

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

	if(proc == NULL)
	{
		return 0;
	}

	int active_tasks = proc->handled_nodes;
	unsigned long long int begin, end, obegin, oend;

	// Make sure everyone starts at the same time
	time_t timeref = time(NULL);
	int done = 0;

#if MONITOR_EXCEPTIONS
        /* Install our signal handler */
        struct sigaction sa;

        sa.sa_sigaction = bt_sighandler;
        sigemptyset (&sa.sa_mask);
        sa.sa_flags = SA_RESTART | SA_SIGINFO;

        sigaction(SIGSEGV, &sa, &oldact[0]);
        sigaction(SIGUSR1, &sa, &oldact[1]);
        sigaction(SIGINT, &sa, &oldact[2]);
        sigaction(SIGFPE, &sa, &oldact[3]);
        sigaction(SIGTERM, &sa, &oldact[4]);
#endif

	// Set frequency of first task
	int freq;
	if(proc->handled_nodes > 0)
	{
		freq = proc->task[0]->frequency;
		drake_platform_set_voltage_frequency(stream->platform, freq);
	}

	/* Phase 1, real */
	while(active_tasks > 0)
	{
		// Capture the stage starting time
		drake_platform_time_get(stream->stage_start_time);
		for(i = 0; i < proc->handled_nodes; i++)
		{
			task = proc->task[i];
			if(task->status < TASK_KILLED)
			{
				// Switch frequency
				if(freq != task->frequency)
				{
					freq = task->frequency;
					drake_platform_set_voltage_frequency(stream->platform, freq);
				}
			}

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

					// Commit
					task_commit(task);
				break;

				case TASK_RUN:
					// Check
					task_check(task);

					// Work
					done = task->run(task);

					// Commit
					task_commit(task);

					if(task->status == TASK_RUN && done)
					{
						
						task->status = TASK_KILLED;
					}

				// Decrement active tasks
				case TASK_KILLED:
					if(task->status == TASK_KILLED)
					{
						task->kill(task);
						task->status = TASK_ZOMBIE;
						active_tasks--;
						//debug_int(active_tasks);

						// Commit
						task_commit(task);
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
					printf("[%s:%s:%d][CORE %zu] Invalid state for task \"%s\".\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), task->name);
					task->status = TASK_ZOMBIE;
				break;

				default:
					printf("[%s:%s:%d][CORE %zu] Unknown state for task \"%s\": %i; switching to invalid state.\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), task->name, task->status);
					task->status = TASK_INVALID;
				break;
			}
		}

		// Pause until the stage time elapses, if any
		if(drake_platform_time_greater(stream->stage_time, stream->zero))
		{
			drake_platform_time_get(stream->stage_stop_time);
			drake_platform_time_subtract(stream->stage_sleep_time, stream->stage_stop_time, stream->stage_start_time);
			if(!drake_platform_time_greater(stream->stage_sleep_time, stream->stage_time))
			{
				drake_platform_time_subtract(stream->stage_sleep_time, stream->stage_time, stream->stage_sleep_time);
				drake_platform_sleep(stream->stage_sleep_time);
			}
		}
	}

	// Get core to sleep
	drake_platform_sleep_enable(stream->platform, drake_platform_core_id());
	//drake_platform_core_disable(stream->platform, drake_platform_core_id());

	return 0;
}

