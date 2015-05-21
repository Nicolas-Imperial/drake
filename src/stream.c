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

#include <snekkja/schedule.h>

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
#include <snekkja/task.h>
#include <snekkja/link.h>
#include <snekkja/cross_link.h>
#include <snekkja/processor.h>
#include <snekkja/mapping.h>
#undef INNER_LINK_BUFFER
#undef INPUT_LINK_BUFFER
#undef OUTPUT_LINK_BUFFER
#undef MPB_SIZE
//#include <scc_printf.h>
#include <snekkja/platform.h>
#include <snekkja/stream.h>
#include <monitor.h>

#include <sort.h>
#include <merge.h>
#include <snekkja/scc.h>

#define PRINTF_TIMEOUT 5
//time_t timeref;
#if DEBUG
//int printf_enabled = 0;
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
//int printf_enabled = 0;
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
//unsigned long long int begin, end, obegin, oend;
#endif

//static unsigned int memory_consistency_errors = 0;

typedef struct {
	size_t argc;
	char **argv;
} snekkja_setup_args_t;

typedef struct {
	char *filename;
} snekkja_setup_tasks_args_t;

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

#if 0
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
#endif

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
		printf_int(cons->id);
		printf_addr(cons->pred);
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

static array_t(task_tp)*
get_task_consumers(mapping_t *mapping, task_t *task)
{
	array_t(task_tp) *consumers;
	/*
	task_t *consumer = find_task(mapping, parent_task_id(task));

	if(consumer != NULL)
	{
		consumers = pelib_alloc_collection(array_t(task_tp))(1);
		pelib_array_append(task_tp)(consumers, consumer);
	}
	else
	{
		consumers = pelib_alloc_collection(array_t(task_tp))(0);
	}
	*/
	size_t i;
	consumers = pelib_alloc_collection(array_t(task_tp))(_snekkja_consumers_in_task[task->id - 1]);
	for(i = 0; i < _snekkja_consumers_in_task[task->id - 1]; i++)
	{
		pelib_array_append(task_tp)(consumers, pelib_mapping_find_task(mapping, _snekkja_consumers_id[task->id - 1][i]));
	}

	return consumers;	
}

static array_t(task_tp)*
get_task_producers(mapping_t *mapping, task_t *task)
{
	array_t(task_tp) *producers;
#if 0
	task_t *left_producer = find_task(mapping, left_child_id(task));
	task_t *right_producer = find_task(mapping, right_child_id(task));

	if(left_producer != NULL && right_producer != NULL)
	{
		producers = pelib_alloc_collection(array_t(task_tp))(2);
		snekkja_stderr("[%s:%s:%d] Adding task %d\n", __FILE__, __FUNCTION__, __LINE__, left_producer->id);
		pelib_array_append(task_tp)(producers, left_producer);
		snekkja_stderr("[%s:%s:%d] Adding task %d\n", __FILE__, __FUNCTION__, __LINE__, right_producer->id);
		pelib_array_append(task_tp)(producers, right_producer);
	}
	else
	{
		producers = pelib_alloc_collection(array_t(task_tp))(0);
	}
#else
	size_t i;
	producers = pelib_alloc_collection(array_t(task_tp))(_snekkja_producers_in_task[task->id - 1]);
	for(i = 0; i < _snekkja_producers_in_task[task->id - 1]; i++)
	{
		size_t task_id = _snekkja_producers_id[task->id - 1][i];
		task_t *prod = pelib_mapping_find_task(mapping, task_id);
		pelib_array_append(task_tp)(producers, prod);
	}
#endif

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
/*
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
*/

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
/*
			target_task = find_task(mapping, parent_task_id(current_task));
			if(target_task != NULL)
			{
				build_link(mapping, proc, current_task, target_task);
			}
*/
		}
	}
}

#if 0
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
#if MEASURE_TASK && 0
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
#endif

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

#if 0
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

	printf_int(task->id);
	printf_addr(task->pred);
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
}
#endif

static
size_t
buffer_size(size_t mpb_size, size_t nb_in, size_t nb_out)
{
	return (mpb_size // total allocable MPB size
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
	snekkja_stderr("MPB size: %u\n", mpb_size);
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

#if 0
#define stack_malloc_printf_addr(addr) fprintf(stderr, "[%s:%s:%d] @%s: %08X\n", __FILE__, __FUNCTION__, __LINE__, #addr, addr);
#define stack_malloc_printf_int(addr) fprintf(stderr, "[%s:%s:%d] %s: %d (signed) = %u (unsigned)\n", __FILE__, __FUNCTION__, __LINE__, #addr, addr, addr);
#else
#define stack_malloc_printf_addr(addr)
#define stack_malloc_printf_int(addr)
#endif
static snekkja_stack_t*
stack_malloc(size_t size)
{
	snekkja_stack_t *stack = (snekkja_stack_t*)malloc(sizeof(snekkja_stack_t));
	stack_malloc_printf_int(size);
	stack->base_ptr = (void*)snekkja_local_malloc(size);
	stack_malloc_printf_addr(stack->base_ptr);
	stack->size = size;
	stack->stack_ptr = (size_t*)malloc(sizeof(size_t) * snekkja_core_size());
	memset(stack->stack_ptr, 0, sizeof(size_t) * snekkja_core_size());
	return stack;
}

#if 0
#define stack_grow_printf_addr(addr) fprintf(stderr, "[%s:%s:%d] @%s: %08X\n", __FILE__, __FUNCTION__, __LINE__, #addr, addr);
#define stack_grow_printf_int(addr) fprintf(stderr, "[%s:%s:%d] %s: %d (signed) = %u (unsigned)\n", __FILE__, __FUNCTION__, __LINE__, #addr, addr, addr);
#else
#define stack_grow_printf_addr(addr)
#define stack_grow_printf_int(addr)
#endif
static void*
stack_grow(snekkja_stack_t *stack, size_t size, int id)
{
	stack_grow_printf_int(size);
	stack_grow_printf_int(id);
	stack_grow_printf_addr(stack->base_ptr);
	stack_grow_printf_int(stack->stack_ptr[id]);
	if((size_t)(stack->stack_ptr[id]) + size <= stack->size)
	{
		size_t ptr = stack->stack_ptr[id];
		stack->stack_ptr[id] += size;
		
		return (void*)((size_t)stack->base_ptr + ptr);
	}
	fprintf(stderr, "[%s:%s:%d] Not enough local memory\n", __FILE__, __FUNCTION__, __LINE__);
	abort();
	return NULL;
}

static int
stack_free(snekkja_stack_t* stack)
{
	free(stack->stack_ptr);
	snekkja_local_free(stack->base_ptr);
	free(stack);

	return 1;
}

#if 1
static
void
allocate_buffers(snekkja_stream_t* stream)
{
	int i, j, k, nb, nb_in, nb_out;
	task_t* task;
	link_t *link;
	cross_link_t *cross_link;
	processor_t *proc;
	//cfifo_init_t init;
	mapping_t *mapping = stream->mapping;
	/*
	snekkja_stderr("[%s:%s:%d] %zX\n", __FILE__, __FUNCTION__, __LINE__, stream->stack);
	snekkja_stderr("[%s:%s:%d] %zX\n", __FILE__, __FUNCTION__, __LINE__, stream->stack->base_ptr);
	snekkja_stderr("[%s:%s:%d] %zX\n", __FILE__, __FUNCTION__, __LINE__, stream->stack->stack_ptr);
	snekkja_stack_t *stack = stream->stack; /**/
	snekkja_stack_t *stack = stack_malloc(stream->local_memory_size);
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
						check_mpb_size(stream->local_memory_size, nb_in_succ, nb_out_succ, proc);

						// If the task is mapped to another core: output link
						if(link->buffer == NULL)
						{
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();
							
							//int core = (int)core_id_in_scc(link->cons->core->id, octant_id(pelib_scc_core_id())); // Transformed
							int core = link->cons->core->id;
							size_t capacity = buffer_size(stream->local_memory_size, nb_in_succ, nb_out_succ) / sizeof(int);
							link->buffer->buffer = (int*)snekkja_remote_addr(stack_grow(stack, sizeof(int) * capacity, core), core);
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
						check_mpb_size(stream->local_memory_size, nb_in, nb_out, proc);
						// If the task is mapped to another core: cross link
						if(link->buffer == NULL)
						{
							// Perform this allocation manually
							link->buffer = pelib_alloc_struct(cfifo_t(int))();

							//int core = (int)core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id())); // Transformed
							int core = task->core->id;
							size_t capacity = buffer_size(stream->local_memory_size, nb_in, nb_out) / sizeof(int);
							link->buffer->buffer = (int*)snekkja_remote_addr(stack_grow(stack, sizeof(int) * capacity, core), core);
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
					//cross_link->read = (volatile size_t*)snekkja_remote_addr(stack_grow(stack, sizeof(size_t), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))); // Transformed
					cross_link->read = (volatile size_t*)snekkja_remote_addr(stack_grow(stack, sizeof(size_t), task->core->id), task->core->id);	
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
					//cross_link->prod_state = (task_status_t*)snekkja_remote_addr(stack_grow(stack, sizeof(enum task_status), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))); // Transformed
					cross_link->prod_state = (task_status_t*)snekkja_remote_addr(stack_grow(stack, sizeof(enum task_status), task->core->id), task->core->id);
					*cross_link->prod_state = TASK_START;
				}

				if(cross_link->write == NULL)
				{
					//cross_link->write = (volatile size_t*)snekkja_remote_addr(stack_grow(stack, sizeof(size_t), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))), core_id_in_scc(task->core->id, octant_id(pelib_scc_core_id()))); // Transformed
					cross_link->write = (volatile size_t*)snekkja_remote_addr(stack_grow(stack, sizeof(size_t), task->core->id), task->core->id);
					*cross_link->write = 0;
				}
				// TODO: call a allocate_output_buffer(task, input_buffer) function. It does nothing for SCC
			}
		}
	}
}
#else
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
					*cross_link->prod_state = TASK_START;
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
#endif

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

static mapping_t*
prepare_mapping()
{
#if 1
	size_t i, j;
	mapping_t *mapping;
	processor_t *processor = NULL;
	task_t task;

	size_t num_cores = _snekkja_p;
	mapping = pelib_alloc_collection(mapping_t)(num_cores);

/*
	for(i = 0; i < 6; i++)
	{
		for(j = 0; j < 32; j++)
		{
			fprintf(stderr, "[%s,%s,%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
			fprintf(stderr, "[%s,%s,%d] core %d task %d\n", __FILE__, __FUNCTION__, __LINE__, i + 1, _snekkja_schedule[i * 32 + j].id);
			fprintf(stderr, "[%s,%s,%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
		}
	}
*/

	for(j = 1; j <= _snekkja_p; j++)
	{
		size_t tasks_in_core = _snekkja_tasks_in_core[j - 1];
		processor = pelib_alloc_collection(processor_t)(tasks_in_core);
		processor->id = j - 1;
		size_t producers_in_core = _snekkja_producers_in_core[j - 1];
		size_t consumers_in_core = _snekkja_consumers_in_core[j - 1];
		processor->source = pelib_alloc_collection(array_t(cross_link_tp))(producers_in_core);
		processor->sink = pelib_alloc_collection(array_t(cross_link_tp))(consumers_in_core);
		pelib_mapping_insert_processor(mapping, processor);

		for(i = 1; i <= _snekkja_tasks_in_core[j - 1]; i++)
		{
			task.id = _snekkja_schedule[j - 1][i - 1].id;
			size_t producers_in_task = _snekkja_producers_in_task[task.id - 1];
			size_t consumers_in_task = _snekkja_consumers_in_task[task.id - 1];
			size_t remote_producers_in_task = _snekkja_remote_producers_in_task[task.id - 1];
			size_t remote_consumers_in_task = _snekkja_remote_consumers_in_task[task.id - 1];
			task.pred = pelib_alloc_collection(array_t(link_tp))(producers_in_task);
			task.succ = pelib_alloc_collection(array_t(link_tp))(consumers_in_task);
			task.source = pelib_alloc_collection(array_t(cross_link_tp))(remote_producers_in_task);
			task.sink = pelib_alloc_collection(array_t(cross_link_tp))(remote_consumers_in_task);

			task.status = TASK_START;

			pelib_mapping_insert_task(mapping, j - 1, &task);
		}
	}
#else
	mapping_t *mapping;
	FILE* file_mapping;
PELIB_SCC_CRITICAL_BEGIN
	// Load mappings
	file_mapping = fopen(mapping_filename, "r");
	if (file_mapping == NULL)
	{
		snekkja_stderr("[CORE %d] Could not open mapping file:%s\n",
		    pelib_scc_core_id(), mapping_filename);
		abort();
	}
	assert_different(file_mapping, NULL, 1);

	mapping = NULL;
	mapping = pelib_mapping_loadfilterfile(mapping, file_mapping, get_left(pelib_scc_core_id()) ? is_left : is_right);
	fclose(file_mapping);
PELIB_SCC_CRITICAL_END
#endif
	return mapping;
}

snekkja_stream_t
snekkja_stream_create(void* aux)
{
	int k;
	char* outputfile;
	array_t(int) *array = NULL;
	snekkja_stream_t stream;

	// Initialize snekkja
	snekkja_arch_init(aux);

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

#if SORT_SEQUENTIAL
	array_t(int) *tmp;
	link_t *link;
	int input_buffer_size, input_size, i;
	unsigned long long int start, stop;

	// Read input size
	tmp = pelib_array_preloadfilenamebinary(int)(argv[2]);
	input_size = pelib_array_length(int)(tmp) / 8;
	pelib_free_struct(array_t(int))(tmp);

	if(core_id_in_scc(snekkja_core(), octant_id(snekkja_core())) == 0)
	{
		array = pelib_array_loadfilenamewindowbinary(int)(argv[2], input_size * octant_id(snekkja_core()), input_size);
	}

#if MEASURE_GLOBAL
	start = rdtsc();
#endif

	if(core_id_in_scc(snekkja_core(), octant_id(snekkja_core())) == 0)
	{
		qsort((char*)array->data, pelib_array_length(int)(array), sizeof(int), greater);
	}
#else
	mapping_t *mapping;

	int i, j, max_nodes;
	int rcce_id;

	processor_t *proc;
	task_t *task;
	link_t* link;

	unsigned long long int start, stop, begin, end;
	snekkja_schedule_init();
	mapping = prepare_mapping();

	// Build task network based on tasks' id
	build_tree_network(mapping);
	//proc = mapping->proc[pelib_mapping_find_processor_index(mapping, core_id_in_octant(snekkja_core()))]; // Transformed
	proc = mapping->proc[pelib_mapping_find_processor_index(mapping, snekkja_core())];
	task = proc->task[0];

	// Initialize task's pointer
	max_nodes = proc->handled_nodes;
	for(i = 0; i < max_nodes; i++)
	{
		task_t *task = proc->task[i];
		task->status = TASK_START;
		task->init = (int (*)(task_t*, void*))snekkja_function(task->id, TASK_INIT);
		task->start = (int (*)(task_t*))snekkja_function(task->id, TASK_START);
		task->run = (int (*)(task_t*))snekkja_function(task->id, TASK_RUN);
		task->destroy = (int (*)(task_t*))snekkja_function(task->id, TASK_KILLED);
		task->frequency = _snekkja_task_frequency[task->id - 1];
	}
	/* Init phase: load input data into tasks */

	stream.mapping = mapping;
	stream.proc = proc;
	stream.local_memory_size = snekkja_arch_local_size() - 32;
	//stream.stage_time = _snekkja_stage_time;
	/**/

	return stream;
}

int
snekkja_stream_init(snekkja_stream_t *stream, void *aux)
{
	int success = 1;
	size_t i;
	size_t max_nodes = stream->proc->handled_nodes;
	
	for(i = 0; i < max_nodes; i++)
	{
		//task_init(proc->task[i], argv[2], mapping);
		//args.mapping = mapping;
		task_t *task = stream->proc->task[i];
		int run = task->init(task, aux);
		success = success && run;
		
	}
	allocate_buffers(stream);

	return success;
}

int
snekkja_stream_destroy(snekkja_stream_t* stream, void* aux)
{
	snekkja_schedule_destroy();
	return snekkja_arch_finalize(aux);
}

int
snekkja_stream_run(snekkja_stream_t* stream)
{
	unsigned long long int start, stop;
	size_t i, j;
	task_t *task;
	mapping_t *mapping = stream->mapping;
	processor_t *proc = stream->proc;
	//allocate_buffers(mapping);
	allocate_buffers(stream);
	int active_tasks = proc->handled_nodes;
	unsigned long long int begin, end, obegin, oend;


	// Make sure everyone starts at the same time

	time_t timeref = time(NULL);

#if MEASURE_GLOBAL
	start = rdtsc();
#endif

	int done = 0;

	/* Phase 1, real */
	while(active_tasks > 0)
	{
		for(i = 0; i < proc->handled_nodes; i++)
		{
			task = proc->task[i];
			switch(task->status)
			{
#if 0
				case TASK_INIT:
					if(task->status == TASK_INIT)
					{
						//task->init(task, NULL);
						//task->status = task_next_state(task);
						task->status = TASK_START;
#if MEASURE_STEPS
						task->start_presort = rdtsc();
#endif
					}
#if MEASURE_STEPS && 0
					end = rdtsc(); task->step_transition += end - begin;
#endif
#endif
				// Checks input and proceeds to start when first input come
				case TASK_START:
					done = task->start(task);
					task_check(task);
					if(task->status == TASK_START && done)
					{
						//task->start(task);
						//task->status = task_next_state(task);
						task->status = TASK_RUN;
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
					// Switch frequency
					if(snekkja_arch_get_frequency() != task->frequency)
					{
						snekkja_arch_set_frequency(task->frequency);
					}
					done = task->run(task);
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
					if(task->status == TASK_RUN && done != 0)
					{
						
						task->status = TASK_KILLED;
						//task->status = task_next_state(task);
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
						//task->destroy(task);
						// Send successor tasks the task killed information
						for(j = 0; j < pelib_array_length(cross_link_tp)(task->sink); j++)
						{
							cross_link_t *link = pelib_array_read(cross_link_tp)(task->sink, j);
							*link->prod_state = task->status;
							snekkja_arch_commit(link->prod_state);
						}
						//pelib_scc_force_wcb(); // Important
						//task->status = task_next_state(task);
						task->status = TASK_ZOMBIE;
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
						//task->status = task_next_state(task);
						task->status = TASK_ZOMBIE;
					}
#if MEASURE_STEPS
					end = rdtsc(); task->step_transition += end - begin;
#endif
				break;

				// Should not happen. If it does happen, let the scheduler decide what to do
				case TASK_INVALID:
					task->status = TASK_ZOMBIE;
				break;

				default:
#if MEASURE_STEPS
					begin = rdtsc();
#endif
					//task->status = task_next_state(task);
					task->status = TASK_INVALID;
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

	return 0;
}
