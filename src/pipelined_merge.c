/* To be defined by compile switches:
 * * MPB_SIZE	Maximum size available on MPB
 * * L2_SIZE maximum size to use on L2 cache
 * * INPUT_BUFFER_SIZE
 * * OUTPUT_BUFFER_SIZE
 * * INNER_BUFFER_SIZE
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

#if DEBUG
/* get REG_EIP from ucontext.h */
//#define __USE_GNU
#include <ucontext.h>
#endif

//#include <RCCE_lib.h>

#include <sync.h>
//#include <printf.h>
//#include <pelib_scc.h>
//#define printf pelib_printf
#include <integer.h>

//enum task_status;
#define INNER_LINK_BUFFER cfifo_t(int)
#define INPUT_LINK_BUFFER enum task_status
#define OUTPUT_LINK_BUFFER enum task_status
#include <task.h>
#include <link.h>
#include <cross_link.h>
#include <processor.h>
#include <mapping.h>
#undef INNER_LINK_BUFFER
#undef INPUT_LINK_BUFFER
#undef OUTPUT_LINK_BUFFER

//#include <scc_printf.h>
#include <architecture.h>
#include <monitor.h>

#include <sort.h>

#define PRINTF_TIMEOUT 5
time_t timeref;
#if DEBUG
int printf_enabled = 0;
#define assert_equal(value, expected, abort_on_failure) if(value != expected) { snekkja_stderr("[CORE %d][%s:%s:%d] Expected %s == %d, got %s == %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #expected, expected, #value, value); if (abort_on_failure) { abort(); } }
#define assert_different(value, expected, abort_on_failure) if(value == expected) { snekkja_stderr("[CORE %d][%s:%s:%d] Got %s == %d, expected different than %s == %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #value, value, #expected, expected); if (abort_on_failure) { abort(); } }
#define assert_geq(value, reference, abort_on_failure) if(value < reference) { snekkja_stderr("[CORE %d][%s:%s:%d] Got %s == %d, strictly lower than than %s == %d, but expected greater or equal (>=\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #value, value, #reference, reference); if (abort_on_failure) { abort(); } }
#define debug snekkja_stdout("[CORE %d][%s:%s:%d] %d out of %d tasks left\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, active_tasks, proc->handled_nodes);
#define debug_task snekkja_stdout("[CORE %d][%s:%s:%d] Task %d state %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, task->id, task->status);
#define debug_task_output snekkja_stdout("[CORE %d][%s:%s:%d] Task %d to task %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, task->id, link->link->task->id);
#define printf_addr(addr) snekkja_stdout("[CORE %d][%s:%s:%d] %s = %X\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #addr, addr)
#define printf_int(integer) snekkja_stdout("[CORE %d][%s:%s:%d] %s = %d (signed), %u (unsigned)\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #integer, integer, integer)
#define printf_str(str) snekkja_stdout("[CORE %d][%s:%s:%d] %s = %s\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #str, str);
#define PRINTF_FEEDBACK 0
#define PRINTF_PUSH 0
#define PRINTF_CHECK_IN 0
#define PRINTF_CHECK_OUT 0
#define CHECK_CORRECT              1
#else
int printf_enabled = 0;
#define assert_equal(value, expected, abort_on_failure)
#define assert_different(value, expected, abort_on_failure)
#define assert_geq(value, reference, abort_on_failure)
#define debug
#define predebug
#define debug_task
#define debug_task_output
#define printf_addr(addr)
#define printf_int(integer)
#define printf_str(str)
#endif

//////////////// printf \\\\\\\\\\\\\\\\\\

#define CANARI 0xdeadbeef
#define EMPTY 0xffffffff

// Tuning option
#define INTERLACE_PRESORT          1
#define WARM_CACHE                 0

#define CHECK_CORRECT              1

#define SORT_SEQUENTIAL            0
#define EXPORT_REFERENCE           0
#define EXPORT_OUTPUT              0

#define MEASURE                    1 // Measure anything at all
#define MEASURE_GLOBAL             1
#define MEASURE_PRESORT            1
#define MEASURE_TASK               1
#define MEASURE_STEPS              1

#if !defined MEASURE || MEASURE == 0
#undef MEASURE
#undef MEASURE_GLOBAL
#undef MEASURE_PRESORT
#undef MEASURE_TASK

#define MEASURE                    0
#define MEASURE_GLOBAL             0
#define MEASURE_PRESORT            0
#define MEASURE_TASK               0
#else
unsigned long long int begin, end, obegin, oend;
#endif

static unsigned int memory_consistency_errors = 0;

static int
int_cmp(const void *a, const void *b)
{
	return *(int*)a - *(int*)b;
}

//////////////// SCC layout \\\\\\\\\\\\\\\\\\

#if 0
static int
core_id_in_octant(int rcce_id)
{
	return rcce_id % 6;
}

static int
quadrant_id(int rcce_id)
{
	return 2 * ((int) (rcce_id / 24)) + (rcce_id / 6) % 2;
}

static int
octant_id(int rcce_id)
{
	return 4 * ((int) (rcce_id / 24)) + 2 * ((rcce_id / 6) % 2) + ((rcce_id / 6)
	    % 4) / 2;
}

static int
core_id_in_scc(int core_id, int octant_id)
{
	return core_id + 6 * (2 * (octant_id % 2 + (int) (octant_id / 4)) + octant_id
	    / 2);
}
#endif

//////////////// Numbers \\\\\\\\\\\\\\\\\\

static int
log2_int(unsigned int n)
{
	unsigned int val;
	unsigned int i;


	i = 0;
	val = n;
	while (val >>= 1)
	{
		i++;
	}

	return i;
}

static unsigned int
lower_32(unsigned int v)
{
	return v - v % 32;
}

static unsigned int
upper_32(unsigned int v)
{
	return lower_32(v + 31);
}

//////////////// Tree \\\\\\\\\\\\\\\\\\

static int
leaf_rank(mapping_t* mapping, task_t* task)
{
	//return (task->id - (1 << mapping->processor_count)) + (quadrant_id(snekkja_core()) * (1 << mapping->processor_count)) - 1; // Transformed
	return (task->id - (1 << mapping->processor_count)) - 1;
}

static int
total_leaves(mapping_t *mapping)
{
	return (1 << (mapping->processor_count - 1)) * 8;
}

static int
parent_task_id(task_t *task)
{
	return ceil(task->id / (float) 2);
}

static int
right_child_id(task_t *task)
{
	return task->id * 2;
}

static int
left_child_id(task_t *task)
{
	return right_child_id(task) - 1;
}

static int
is_left(task_t* task)
{
	int id = task->id - 1;

	if (id == 0 || id == 1)
	{
		return 0;
	}
	else
	{
		return ((id - (1 << log2_int(id))) >= (1 << (log2_int(id) - 1)));
	}
}

static int
is_right(task_t* task)
{
	int id = task->id - 1;

	if (id == 0 || id == 1)
	{
		return 0;
	}
	else
	{
		return !is_left(task);
	}
}

static int
get_left(int id)
{
	return (id / 12) % 2 == 1;
}

static
task_t*
find_task(mapping_t* mapping, task_id id)
{
	int target_task_rank, target_proc_rank;
	processor_t *target_proc;

	target_proc_rank = pelib_mapping_find_processor(mapping, id);
	if(target_proc_rank >= 0)
	{
		target_proc = mapping->proc[target_proc_rank];
		target_task_rank = pelib_processor_find_task(target_proc, id);

		if(target_task_rank >= 0)
		{
			return target_proc->task[target_task_rank];
		}
	}
	
	return NULL;
}

/*
static
int
check_input(char* buffer, size_t size, int size_elem, int canari)
{
	size_t i;

	for(i = 0; i < size / size_elem; i++)
	{
		if(((int*)buffer)[i] == canari)
		{
			return -1;
		}
	}

	return 1;
}
*/

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
		link = (link_t*)malloc(sizeof(link_t));
		link->prod = prod;
		link->cons = cons;
		link->buffer = NULL;

		// Add it as source and sink to both current and target tasks
		pelib_array_append(link_tp)(cons->pred, link);
		pelib_array_append(link_tp)(prod->succ, link);

		// If tasks are mapped to different cores
		if(prod->core != cons->core)
		{
			cross_link = (cross_link_t*)malloc(sizeof(cross_link_t));
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

/*
static array_t(task_t)
get_task_producers(task_t *task)
{
	array_t(task_t) *producers;
	task_t *producer = find_task(mapping, parent_task_id(task));

	if(producer != NULL)
	{
		producers = pelib_alloc_collection(array_t(task_t))(1);
		pelib_array_append(task_t)(*producer);
	}
	else
	{
		producers = pelib_alloc_collection(array_t(task_t))(0);
	}

	return producers;	
}
*/

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

			
/* TODO: Build links after taskgraph instead of task ids */
			target_task = find_task(mapping, left_child_id(current_task));
			if(target_task != NULL)
			{
				build_link(mapping, proc, target_task, current_task);
			}

			target_task = find_task(mapping, right_child_id(current_task));
			if(target_task != NULL)
			{
				build_link(mapping, proc, target_task, current_task);
			}

			target_task = find_task(mapping, parent_task_id(current_task));
			if(target_task != NULL)
			{
				build_link(mapping, proc, current_task, target_task);
			}
		}
	}
}

static
task_status_t
task_next_state(task_t* task)
{
	int i, all_killed, all_empty, all_pushed;
	link_t* link;

	switch(task->status)
	{
		case TASK_INIT:
			return TASK_START;
		break;

		case TASK_START:
#if MEASURE_TASK || 1
			all_empty = 1; // Actually means "None is empty"
			all_killed = 1;

			//pelib_scc_cache_invalidate(); // Not important?

			for(i = 0; i < pelib_array_length(link_tp)(task->pred); i++)
			{
				link = pelib_array_read(link_tp)(task->pred, i);
				all_empty = all_empty && !pelib_cfifo_is_empty(int)(link->buffer);
				all_killed = all_killed && (link->prod == NULL ? 1 : link->prod->status >= TASK_KILLED);
			}

			if(!all_empty && !all_killed)
			{
				return TASK_START;
			}

			return TASK_RUN;
#else
			return TASK_RUN;
#endif
		break;

		case TASK_RUN:
			all_empty = 1;
			all_killed = 1;
			all_pushed = 1;

			for(i = 0; i < pelib_array_length(link_tp)(task->pred); i++)
			{
				link = pelib_array_read(link_tp)(task->pred, i);
				all_empty = all_empty && pelib_cfifo_is_empty(int)(link->buffer);
				all_killed = all_killed && (link->prod == NULL ? 1 : link->prod->status >= TASK_KILLED);
			}

			for(i = 0; i < pelib_array_length(cross_link_tp)(task->sink); i++)
			{
				link = pelib_array_read(cross_link_tp)(task->sink, i)->link;
				all_pushed = all_pushed && (link->cons == NULL ? 1 : pelib_cfifo_is_empty(int)(link->buffer));
			}

			if(all_empty)
			{
				if(all_killed)
				{
					if(all_empty && all_killed && all_pushed)
					{
						return TASK_KILLED;
					}
				}
			}

			return TASK_RUN;
		break;

		case TASK_KILLED:
			return TASK_ZOMBIE;
		break;

		case TASK_ZOMBIE:
			return TASK_ZOMBIE;
		break;

		case TASK_INVALID:
		default:
			snekkja_stderr("[CORE %d] Invalid state for task %u (state %u), killing it\n", snekkja_core(), task->id, task->status);
			return TASK_KILLED;
		break;
	}

	return TASK_INVALID;
}

static
void
feedback_link(task_t *task, cross_link_t *link)
{
	enum task_status new_state;
	size_t size;

	//RC_cache_invalidate(); // Not important?
	size = link->available - pelib_cfifo_length(int)(*link->link->buffer);

	if(size > 0)
	{
		//RC_cache_invalidate(); // Not important?
#if PRINTF_FEEDBACK
if(printf_enabled & 1) {
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
		/* Reset all values read to CANARI */
		/*
		size_t i;
		for(i = 0; i < size; i++)
		{
			RC_cache_invalidate();
			//printf("[%s:%s:%d] Resetting task %d fifo buffer at index %d / %d: %d (%d)\n", __FILE__, __FUNCTION__, __LINE__, task->id, (link->link->buffer->capacity + link->link->buffer->read - i - 1) % link->link->buffer->capacity, link->link->buffer->capacity, link->link->buffer->buffer[(link->link->buffer->capacity + link->link->buffer->read - i - 1) % link->link->buffer->capacity], CANARI);
			RC_cache_invalidate();
			link->link->buffer->buffer[(link->link->buffer->capacity + link->link->buffer->read - i - 1) % link->link->buffer->capacity] = (int)CANARI;
			RC_cache_invalidate();
			//printf("[%s:%s:%d] Resetting task %d fifo buffer at index %d / %d: %d (%d)\n", __FILE__, __FUNCTION__, __LINE__, task->id, (link->link->buffer->capacity + link->link->buffer->read - i - 1) % link->link->buffer->capacity, link->link->buffer->capacity, link->link->buffer->buffer[(link->link->buffer->capacity + link->link->buffer->read - i - 1) % link->link->buffer->capacity], CANARI);
			RC_cache_invalidate();
		}
		*/

		link->total_read += size;
		//pelib_scc_cache_invalidate(); // Not important ?
		*link->read = link->total_read;
		pelib_scc_force_wcb(); // Important
		link->available = pelib_cfifo_length(int)(*link->link->buffer);
#if PRINTF_FEEDBACK
if(printf_enabled & 1) {
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
	//RC_cache_invalidate(); // Not important?
	size_t length = pelib_cfifo_length(int)(*link->link->buffer);
	size_t size = length - link->available;

	if(size > 0)
	{
		//RC_cache_invalidate(); // Not important?
#if PRINTF_PUSH
if(printf_enabled & 2) {
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
		pelib_scc_force_wcb(); // Important
		link->available = pelib_cfifo_length(int)(*link->link->buffer);
#if PRINTF_PUSH
if(printf_enabled & 2) {
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
	pelib_scc_cache_invalidate(); // Important
	// Update input fifo length
	size_t write = *link->write - link->total_written;
	size_t actual_write = 0;

	/*
	size_t i;
	int ok = 1;
	for(i = 0; i < write; i++)
	{
		snekkja_stdout("[%s:%s:%d] Checking task %d fifo buffer at index %d / %d: %d (%d)\n", __FILE__, __FUNCTION__, __LINE__, task->id, (link->link->buffer->write + i) % link->link->buffer->capacity, link->link->buffer->capacity, link->link->buffer->buffer[(link->link->buffer->write + i) % link->link->buffer->capacity], CANARI);
		if((int)(link->link->buffer->buffer[(link->link->buffer->write + i) % link->link->buffer->capacity]) == (int)CANARI)
		{
			snekkja_stdout("[%s:%s:%d] Found a canari for task %d fifo buffer at index %d / %d: %d (%d)\n", __FILE__, __FUNCTION__, __LINE__, task->id, (link->link->buffer->write + i) % link->link->buffer->capacity, link->link->buffer->capacity, link->link->buffer->buffer[(link->link->buffer->write + i) % link->link->buffer->capacity], CANARI);
			ok = 0;
		}
	}

	if(ok)
	{*/
		actual_write = pelib_cfifo_fill(int)(link->link->buffer, write);
		link->actual_written = actual_write;
	//}

	if(actual_write > 0)
	{
		//RC_cache_invalidate(); // Not important?
#if PRINTF_CHECK_IN
if(printf_enabled & 4) {
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
		link->available = pelib_cfifo_length(int)(*link->link->buffer);
		link->total_written += write;
#if PRINTF_CHECK_IN
if(printf_enabled & 4) {
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
		if(printf_enabled & 16) {
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
	pelib_scc_cache_invalidate(); // Important
	// Update input fifo length
	size_t read = *link->read - link->total_read;
	size_t actual_read = pelib_cfifo_discard(int)(link->link->buffer, read);
	link->actual_read = actual_read;

	if(actual_read > 0)
	{
		//RC_cache_invalidate(); // Not important?
#if PRINTF_CHECK_OUT
		if(printf_enabled & 8) {
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
		link->available = pelib_cfifo_length(int)(*link->link->buffer);
		link->total_read += read;
#if PRINTF_CHECK_OUT
		if(printf_enabled & 8) {
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
	}
}

static
void
task_check(task_t *task)
{
	int i;

	// TODO: RC_cache_invalidate() or not?
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
void
task_init(task_t *task, char *input_filename, mapping_t *mapping)
{
	/* if task has no predecessor, then it creates two tasks
	for which it loads a portion of input and sort it, until it obtains
	two sorted subsequences that can be read from as if it was a fifo */
	link_t *link;
	array_t(int) *tmp;
	int input_buffer_size, input_size, i;

	// Read input size
	tmp = pelib_array_preloadfilenamebinary(int)(input_filename);
	input_size = pelib_array_length(int)(tmp);
	pelib_free_struct(array_t(int))(tmp);

	// Reset performance counters
#if MEASURE
	task->start_time = 0;
	task->stop_time = 0;
	task->start_presort = 0;
	task->stop_presort = 0;
	task->step_init = 0;
	task->step_start = 0;
	task->step_check = 0;
	task->step_work = 0;
	task->step_push = 0;
	task->step_killed = 0;
	task->step_zombie = 0;
	task->step_transition = 0;
#endif

	// If the task has no predecessor (is a leaf)
	if(pelib_array_length(link_tp)(task->pred) == 0)
	{
		input_buffer_size = input_size / total_leaves(mapping) / 2;

		for(i = 0; i < 2; i++)
		{
			link = (link_t*)malloc(sizeof(link_t));
			link->prod = NULL;
			link->cons = task;

			link->buffer = (cfifo_t(int)*)pelib_array_loadfilenamewindowbinary(int)(input_filename, 2 * input_buffer_size * leaf_rank(mapping, task) + input_buffer_size * i, input_buffer_size);
			pelib_array_append(link_tp)(task->pred, link);
		}
	}

	if(pelib_array_length(link_tp)(task->succ) == 0) // root node
	{
		link = (link_t*)malloc(sizeof(link_t));
		size_t capacity = (int)ceil((double)input_size) / 8;
		//init.core = -1;
		link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
		link->cons = NULL;
		link->prod = task;
		pelib_init(cfifo_t(int))(link->buffer);
		pelib_array_append(link_tp)(task->succ, link);
	}

	task->status = TASK_INIT;
}

static
size_t
buffer_size(size_t mpb_size, size_t nb_in, size_t nb_out)
{
	return (MPB_SIZE // total allocable MPB size
		- nb_in // As many input buffer as
			* (sizeof(size_t)  // Size of a write
				+ sizeof(enum task_status) // State of a task
			)
		- nb_out // As many output buffers as
			* sizeof(size_t) // Size of a read
		)
	 / nb_in; // Remaining space to be divided by number of input links
}

static
void
printf_mpb_allocation(size_t mpb_size, size_t nb_in, size_t nb_out)
{
	snekkja_stderr("MPB size: %u\n", MPB_SIZE);
	snekkja_stderr("Input links: %u\n", nb_in);
	snekkja_stderr("Output links: %u\n", nb_out);
	snekkja_stderr("Total allocable memory per input link: %u\n", buffer_size(mpb_size, nb_in, nb_out < 1));
}

static
void
error_small_mpb(size_t mpb_size, size_t nb_in, size_t nb_out, processor_t *proc)
{
	snekkja_stderr("Too much cross core links at processor %u\n", proc->id);
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
allocate_buffers(mapping_t* mapping)
{
	int i, j, k, nb, nb_in, nb_out;
	task_t* task;
	link_t *link;
	cross_link_t *cross_link;
	processor_t *proc;
	//cfifo_init_t init;
	mpb_stack_t *stack = pelib_scc_stack_malloc(MPB_SIZE);	
	//init.stack = stack;

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
							//init.core = -1;
							size_t capacity = INNER_BUFFER_SIZE / proc->inner_links / sizeof(int);

							link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
					else
					{
						size_t nb_in_succ = pelib_array_length(cross_link_tp)(link->cons->core->source);
						size_t nb_out_succ = pelib_array_length(cross_link_tp)(link->cons->core->sink);
						check_mpb_size(MPB_SIZE, nb_in_succ, nb_out_succ, proc);

						// If the task is mapped to another core: output link
						if(link->buffer == NULL)
						{
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();
							
							//int core = (int)core_id_in_scc(link->cons->core->id, octant_id(pelib_scc_core_id())); // Transformed
							int core = link->cons->core->id;
							size_t capacity = buffer_size(MPB_SIZE, nb_in_succ, nb_out_succ) / sizeof(int);
							link->buffer->buffer = (int*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(int) * capacity, core), core);
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
							//init.core = -1;
							size_t capacity = INNER_BUFFER_SIZE / proc->inner_links / sizeof(int);

							link->buffer = pelib_alloc_collection(cfifo_t(int))(capacity);
							pelib_init(cfifo_t(int))(link->buffer);
						}
					}
					else
					{
						nb_in = pelib_array_length(cross_link_tp)(proc->source);
						nb_out = pelib_array_length(cross_link_tp)(proc->sink);
						check_mpb_size(MPB_SIZE, nb_in, nb_out, proc);
						// If the task is mapped to another core: cross link
						if(link->buffer == NULL)
						{
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();

							//int core = (int)core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id())); // Transformed
							int core = task->core->id;
							size_t capacity = buffer_size(MPB_SIZE, nb_in, nb_out) / sizeof(int);
							link->buffer->buffer = (int*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(int) * capacity, core), core);
							link->buffer->capacity = capacity;
							pelib_init(cfifo_t(int))(link->buffer);

						/*
							link->buffer = pelib_alloc(cfifo_t(int))(&init);
							pelib_init(cfifo_t(int))(link->buffer);
*/
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
					//cross_link->read = (volatile size_t*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(size_t), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))); // Transformed
					cross_link->read = (volatile size_t*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(size_t), task->core->id), task->core->id);	
					*cross_link->read = 0;
				}

				// TODO: call a allocate_output_buffer(task, output_buffer) function doing what's below
			}

			// Take care of all input links
			for(k = 0; k < pelib_array_length(cross_link_tp)(task->source); k++)
			{
				cross_link = pelib_array_read(cross_link_tp)(task->source, k);

				if(cross_link->prod_state == NULL)
				{
					//cross_link->prod_state = (task_status_t*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(enum task_status), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))); // Transformed
					cross_link->prod_state = (task_status_t*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(enum task_status), task->core->id), task->core->id);
					*cross_link->prod_state = TASK_INIT;
				}

				if(cross_link->write == NULL)
				{
					//cross_link->write = (volatile size_t*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(size_t), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))); // Transformed
					cross_link->write = (volatile size_t*)pelib_scc_global_ptr(pelib_scc_stack_grow(stack, sizeof(size_t), task->core->id), task->core->id);
					*cross_link->write = 0;
				}
				// TODO: call a allocate_output_buffer(task, input_buffer) function. It does nothing for SCC
			}
		}
	}
}

	int mem[300];
static
void
merge(task_t *task)
{
	link_t *left, *right, *parent;
	left = pelib_array_read(link_tp)(task->pred, 0);
	right = pelib_array_read(link_tp)(task->pred, 1);
	parent = pelib_array_read(link_tp)(task->succ, 0);

/*
	size_t s;
	s = pelib_cfifo_discard(int)(left->buffer, pelib_cfifo_length(int)(*left->buffer));
	s += pelib_cfifo_discard(int)(right->buffer, pelib_cfifo_length(int)(*right->buffer));
	pelib_cfifo_fill(int)(parent->buffer, s);
*/

	size_t capacity = pelib_cfifo_capacity(int)(parent->buffer);
	size_t length = pelib_cfifo_length(int)(*parent->buffer);
	size_t max_push = capacity - length;
	size_t max_left = pelib_cfifo_length(int)(*left->buffer);
	size_t max_right = pelib_cfifo_length(int)(*right->buffer);
	size_t read_left = 0, read_right = 0, pushed = 0;
	int left_val, right_val;
	
	while(max_push > 0 && max_left > 0 && max_right > 0)
	{
		left_val = left->buffer->buffer[(left->buffer->read + read_left) % left->buffer->capacity];
		right_val = right->buffer->buffer[(right->buffer->read + read_right) % right->buffer->capacity];

		if(left_val < right_val)
		{
			read_left++;
			max_left--;
			parent->buffer->buffer[(parent->buffer->write + pushed) % parent->buffer->capacity] = left_val;
		}
		else
		{
			read_right++;
			max_right--;
			parent->buffer->buffer[(parent->buffer->write + pushed) % parent->buffer->capacity] = right_val;
		}

		pushed++;
		max_push--;
	}

	// Update state for fifos
	pelib_cfifo_discard(int)(left->buffer, read_left);
	pelib_cfifo_discard(int)(right->buffer, read_right);
	pelib_cfifo_fill(int)(parent->buffer, pushed);

	// Flush merge
	int left_killed = ((left->prod == NULL) ? 1 : left->prod->status >= TASK_KILLED);
	int left_empty = pelib_cfifo_is_empty(int)(left->buffer);

	if(left_killed)
	{
		if(left_empty)
		{
			// Copy from fifo to fifo
			pushed = pelib_cfifo_popfifo(int)(right->buffer, parent->buffer, pelib_cfifo_length(int)(*right->buffer));
		}
	}

	int right_killed = ((right->prod == NULL) ? 1 : right->prod->status >= TASK_KILLED);
	int right_empty = pelib_cfifo_is_empty(int)(right->buffer);

	if(right_killed)
	{
		if(right_empty)
		{
			// Copy from fifo to fifo
			pushed = pelib_cfifo_popfifo(int)(left->buffer, parent->buffer, pelib_cfifo_length(int)(*left->buffer));
		}
	}

}

int
greater(const void *a, const void *b)
{
	return *(int*)a - *(int*)b;
}

static
void
task_presort(task_t *task)
{
	link_t *link;
	int j;

	for(j = 0; j < pelib_array_length(link_tp)(task->pred); j++)
	{
		link = pelib_array_read(link_tp)(task->pred, j);

		if(link->prod == NULL)
		{
			array_t(int) *array;
			array = (array_t(int)*)link->buffer;
			qsort((char*)array->data, pelib_array_length(int)(array), sizeof(int), greater);
			// Change array to fifo
			link->buffer = pelib_cfifo_from_array(int)((array_t(int)*)link->buffer);
		}
	}

}

#if DEBUG 
void
printf_link(cross_link_t *link)
{
	printf_int(link->link->buffer->read);
	printf_int(link->link->buffer->write);
	printf_int(link->link->buffer->last_op);
	printf_int(pelib_cfifo_length(int)(*link->link->buffer));
	printf_int(pelib_cfifo_capacity(int)(link->link->buffer));	

	printf_int(link->available);
	printf_int(link->total_read);
	printf_int(link->total_written);

	printf_int(link->prod_state);
	printf_int(link->actual_read);
	printf_int(link->actual_written);

	snekkja_arch_pull(link->read);
	printf_int(*link->read);
	snekkja_arch_pull(link->write);
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
	snekkja_barrier(NULL);
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
	snekkja_barrier(NULL);
	printf_str("Resuming computation");

	/* Do something useful with siginfo_t */
	if (sig == SIGSEGV)
	{
		snekkja_stdout("Got signal %d, faulty address is %p, from %p\n", sig, info->si_addr, uc->uc_mcontext.gregs[REG_EIP]);
	}
	else
	{
		snekkja_stdout("Got signal %d\n", sig);
	}

	trace_size = backtrace(trace, 16);
	/* overwrite sigaction with caller's address */
	trace[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];

	messages = backtrace_symbols(trace, trace_size);
	/* skip first stack frame (points here) */
	snekkja_stdout("[bt] Execution path:\n");
	for (i = 1; i < trace_size; i++)
	{
		snekkja_stdout("[bt] %s\n", messages[i]);
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

struct args
{
	int *argc;
	char ***argv;
};
typedef struct args args_t;

int
main(int argc, char **argv)
{
	int k;
	char* outputfile;
	array_t(int) *array = NULL;

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);

	// Initialize snekkja
	args_t args;
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	args.argc = &argc;
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	args.argv = &argv;
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	snekkja_arch_init((void*)&args);
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);

#if !DEBUG 
	// Initialize and redirect pelib's standard output
	//pelib_scc_init_redirect();
	//pelib_scc_set_redirect();
#else
	/* Install our signal handler */
	struct sigaction sa;

	sa.sa_sigaction = bt_sighandler;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
#endif

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
#if SORT_SEQUENTIAL
	array_t(int) *tmp;
	link_t *link;
	int input_buffer_size, input_size, i;
	unsigned long long int start, stop;

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	// Read input size
	tmp = pelib_array_preloadfilenamebinary(int)(argv[2]);
	input_size = pelib_array_length(int)(tmp) / 8;
	pelib_free_struct(array_t(int))(tmp);

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	if(core_id_in_scc(snekkja_core(), octant_id(snekkja_core())) == 0)
	{
		array = pelib_array_loadfilenamewindowbinary(int)(argv[2], input_size * octant_id(snekkja_core()), input_size);
	}

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
#if MEASURE_GLOBAL
	start = rdtsc();
#endif

	if(core_id_in_scc(snekkja_core(), octant_id(snekkja_core())) == 0)
	{
		qsort((char*)array->data, pelib_array_length(int)(array), sizeof(int), greater);
	}
#else
	mapping_t *mapping;
	FILE* file_mapping;

	int i, j, max_nodes;
	int rcce_id;

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	processor_t *proc;
	task_t *task;
	link_t* link;

	unsigned long long int start, stop, begin, end;

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
PELIB_SCC_CRITICAL_BEGIN
	// Load mappings
	file_mapping = fopen(argv[1], "r");
	if (file_mapping == NULL)
	{
		snekkja_stderr("[CORE %d] Could not open mapping file:%s\n",
		    pelib_scc_core_id(), argv[1]);
		abort();
	}
	assert_different(file_mapping, NULL, 1);

	mapping = NULL;
	mapping = pelib_mapping_loadfilterfile(mapping, file_mapping, get_left(pelib_scc_core_id()) ? is_left : is_right);
	fclose(file_mapping);
PELIB_SCC_CRITICAL_END

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	// Build task network based on tasks' id
	build_tree_network(mapping);
	//proc = mapping->proc[pelib_mapping_find_processor_index(mapping, core_id_in_octant(snekkja_core()))]; // Transformed
	proc = mapping->proc[pelib_mapping_find_processor_index(mapping, snekkja_core())];
	task = proc->task[0];

	/* Init phase: load input data into tasks */
PELIB_SCC_CRITICAL_BEGIN
	max_nodes = proc->handled_nodes;
	for(i = 0; i < max_nodes; i++)
	{
		task_init(proc->task[i], argv[2], mapping);
	}
PELIB_SCC_CRITICAL_END
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	allocate_buffers(mapping);
	int active_tasks = proc->handled_nodes;

	// Make sure everyone starts at the same time
	snekkja_barrier(NULL);

	timeref = time(NULL);

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
#if MEASURE_GLOBAL
	start = rdtsc();
#endif
#if !INTERLACE_PRESORT
	/* Phase 0: sequential sorting of all input buffers */
	for(i = 0; i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		task_presort(task);
	}		
#endif
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	/* Phase 1, real */
	while(active_tasks > 0)
	{
		for(i = 0; i < proc->handled_nodes; i++)
		{
			task = proc->task[i];
			switch(task->status)
			{
				case TASK_INIT:
					if(task->status == TASK_INIT)
					{
						task->status = task_next_state(task);
#if MEASURE_STEPS
						task->start_presort = rdtsc();
#endif
					}
#if MEASURE_STEPS && 0
					end = rdtsc(); task->step_transition += end - begin;
#endif
				// Checks input and proceeds to start when first input come
				case TASK_START:
#if INTERLACE_PRESORT
					task_presort(task);
#endif
					task_check(task);
					if(task->status == TASK_START)
					{
						task->status = task_next_state(task);
					}
#if INTERLACE_PRESORT
					// Start this task right away if input are available
					if(task->status == TASK_START)
					{
#endif
						break;
#if INTERLACE_PRESORT
					}
#endif
#if INTERLACE_PRESORT

#if MEASURE_TASK
					// If the program reaches this point, then the task is started
					task->stop_presort = rdtsc();
#endif
#endif
#if MEASURE_TASK
					task->start_time = rdtsc();
#endif
				case TASK_RUN:
#if MEASURE_STEPS
					begin = rdtsc();
#endif
					// Check
					task_check(task);
#if MEASURE_STEPS
					end = rdtsc(); task->step_check += end - begin;
#endif

#if MEASURE_STEPS
					begin = rdtsc();
#endif
					// Work
					merge(task);
#if MEASURE_STEPS
					end = rdtsc(); task->step_work += end - begin;
#endif

#if MEASURE_STEPS
					begin = rdtsc();
#endif
					// Commit
					task_commit(task);
#if MEASURE_STEPS
					end = rdtsc(); task->step_push += end - begin;
#endif

#if MEASURE_STEPS
					begin = rdtsc();
#endif
					if(task->status == TASK_RUN)
					{
						task->status = task_next_state(task);
					}
#if MEASURE_STEPS
					end = rdtsc(); task->step_transition += end - begin;
#endif
				// Stop stopwatch and decrement active tasks
				case TASK_KILLED:
					if(task->status == TASK_KILLED)
					{
#if MEASURE_STEPS
						begin = rdtsc();
#endif
						// Send successor tasks the task killed information
						for(j = 0; j < pelib_array_length(cross_link_tp)(task->sink); j++)
						{
							cross_link_t *link = pelib_array_read(cross_link_tp)(task->sink, j);
							*link->prod_state = task->status;
						}
						pelib_scc_force_wcb(); // Important
						task->status = task_next_state(task);
#if MEASURE_STEPS
						end = rdtsc(); task->step_transition += end - begin;
#endif

#if MEASURE_STEPS
						begin = rdtsc();
#endif
#if MEASURE_TASK
						task->stop_time = rdtsc();
#endif
						active_tasks--;
#if MEASURE_STEPS
						end = rdtsc(); task->step_killed += end - begin;
#endif
					}
				break;

				// Loop until all other tasks are also done
				case TASK_ZOMBIE:
#if MEASURE_STEPS
					begin = rdtsc();
#endif
					if(task->status == TASK_ZOMBIE)
					{
						task->status = task_next_state(task);
					}
#if MEASURE_STEPS
					end = rdtsc(); task->step_transition += end - begin;
#endif
				break;

				// Should not happen. If it does happen, let the scheduler decide what to do
				default:
#if MEASURE_STEPS
					begin = rdtsc();
#endif
					task->status = task_next_state(task);
#if MEASURE_STEPS
					end = rdtsc(); task->step_transition += end - begin;
#endif
				break;
			}
		}
	}
#endif

	// Phase 3 here?
#if MEASURE_GLOBAL
	stop = rdtsc();
#endif

#if CHECK_CORRECT
	int final_size;
	array_t(int) *sorted, *ref;
	int error_detected = 0;
	int got = 0;

#if !SORT_SEQUENTIAL
	for(i = 0; i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		for(j = 0; j < pelib_array_length(link_tp)(task->succ); j++)
		{
			link = pelib_array_read(link_tp)(task->succ, j);

			if(link->cons == NULL)
			{
				array = pelib_array_from_cfifo(int)(link->buffer);
			}
		}
	}
#endif

	PELIB_SCC_CRITICAL_BEGIN
	if(array != NULL)
	{
		// Load input and qsort it using RCCE_qsort
		ref = pelib_array_preloadfilenamebinary(int)(argv[2]);
		final_size = pelib_array_length(int)(ref);
		pelib_free_struct(array_t(int))(ref);
		final_size = final_size / 8;

		if(final_size != pelib_array_length(int)(array))
		{
			snekkja_stderr("[CORE %d][ERROR] The length of sorted array (%d) is different than expected length (%d)\n", snekkja_core(), pelib_cfifo_length(int)(*link->buffer), final_size);
			abort();
			error_detected = 1;
		}
		//ref = pelib_array_loadfilenamewindowbinary(int)(argv[2], final_size * octant_id(snekkja_core()), final_size); // Transformed
		ref = pelib_array_loadfilenamewindowbinary(int)(argv[2], 0, final_size);

		// qsort so we can compare our output to a correct output
		qsort((char*)ref->data, final_size, sizeof(int), int_cmp);

		for(k = 0; k < final_size; k++)
		{
			if(ref->data[k] != array->data[k])
			{
				//abort();
				error_detected = 1;
				got = array->data[k];
				break;
			}
		}
#if EXPORT_REFERENCE
		outputfile = malloc(strlen(argv[2]) + 4 + 3);
		sprintf(outputfile, "%s.ref.%d", argv[2], octant_id(snekkja_core()));
		pelib_array_storefilename(int)(ref, outputfile);
		free(outputfile);
#endif
	}

#if EXPORT_OUTPUT
	if(array != NULL)
	{
		umask(022);
		// Save output file to disk
		outputfile = malloc(strlen(argv[2]) + 3);
		sprintf(outputfile, "%s.%d", argv[2], octant_id(snekkja_core()));
		pelib_array_storefilename(int)(array, outputfile);
		free(outputfile);
	}
#endif

	PELIB_SCC_CRITICAL_END
	snekkja_barrier(NULL);

	if(!error_detected)
	{
		snekkja_stderr("[CORE %d] Everything went OK\n", snekkja_core());
	}
	else
	{
		snekkja_stderr("[CORE %d][ERROR] The sorted array did not match reference at index %d (got %d, expected %d)\n", snekkja_core(), k, got, ref->data[k]);
	}
#endif
	snekkja_stderr("[CORE %d] Now finishing...\n", snekkja_core());

#if MEASURE
#if !SORT_SEQUENTIAL
	snekkja_stdout("%% table_columns = \
		core \
		proc_id \
		octant_id \
		errors \
		task_id \
		start \
		stop \
		presort_start \
		presort_stop \
		task_start \
		task_stop \
		task_step_init \
		task_step_start \
		task_step_check \
		task_step_work \
		task_step_push \
		task_step_killed \
		task_step_zombie \
		task_step_transition \
	\n");
	for(i = 0; i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		snekkja_stdout(
			"%d %d %d %d %d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
			snekkja_core(),
			proc->id,
			//octant_id(snekkja_core()), // Transformed
			0,
			memory_consistency_errors,
			task->id,
			start,
			stop,
			task->start_presort,
			task->stop_presort,
			task->start_time,
			task->stop_time,
			task->step_init,
			task->step_start,
			task->step_check,
			task->step_work,
			task->step_push,
			task->step_killed,
			task->step_zombie,
			task->step_transition
		);
	}
#else
	if(core_id_in_octant(snekkja_core()) == 0)
	{
		snekkja_stdout("%% table_columns = \
			core \
			proc_id \
			octant_id \
			start \
			stop \
		\n");
		snekkja_stdout(
			"%d %d %d %llu %llu\n",
			snekkja_core(),
			0,
			octant_id(snekkja_core()),
			start,
			stop
		);
	}
#endif
#endif
	
	//pelib_scc_stop_redirect();
	//pelib_scc_finalize_redirect();

	snekkja_arch_finalize(NULL);

	return error_detected;
	return 0;
}

