#include <stddef.h>

#ifndef TASK_H
#define TASK_H

enum task_status {TASK_INVALID, TASK_INIT, TASK_START, TASK_CHECK, TASK_WORK, TASK_PUSH, TASK_RUN, TASK_KILLED, TASK_ZOMBIE};
typedef enum task_status task_status_t;

typedef unsigned int task_id;

// Forward declarations for links
struct link;
typedef struct link link_t;
typedef link_t* link_tp;

#define STRUCT_T link_t
#include <structure.h>
#define DONE_link_t 1

#define STRUCT_T link_tp
#include <structure.h>
#define DONE_link_tp 1

#define ARRAY_T link_tp
#include <array.h>
#define DONE_array_link_tp 1

// Forward declarations for cross links
struct cross_link;
typedef struct cross_link cross_link_t;
typedef cross_link_t* cross_link_tp;

#define STRUCT_T cross_link_t
#include <structure.h>
#define DONE_cross_link_t 1

#define STRUCT_T cross_link_tp
#include <structure.h>
#define DONE_cross_link_tp 1

#define ARRAY_T cross_link_tp
#include <array.h>
#define DONE_array_cross_link_tp 1

struct processor;
typedef struct processor processor_t;

struct task
{
	/// Identification of the task. All tasks (say n tasks) should be numbered continuously from 1 to n to be friendly with files generated to ILP solver and task graph graphic generator.
	task_id id;
	processor_t* core;
	array_t(link_tp) *pred, *succ;
	array_t(cross_link_tp) *sink;
	array_t(cross_link_tp) *source;
	task_status_t status;
	unsigned long long int start_time, stop_time, start_presort, stop_presort, check_time, push_time, work_time, check_errors, check_recv, check_putback, check_feedback, put_reset, put_pop, put_send, check_wait, push_wait, work_wait, work_read, work_write;
	unsigned long long int step_init, step_start, step_check, step_work, step_push, step_killed, step_zombie, step_transition;
};
typedef struct task task_t;
typedef task_t* task_tp;

#define STRUCT_T task_t
#include <structure.h>
#define DONE_task_t 1

#define STRUCT_T task_tp
#include <structure.h>
#define DONE_task_tp 1

#define ARRAY_T task_tp
#include <array.h>
#define DONE_array_task_tp 1

#if 0
extern const char* status_name[];
typedef volatile unsigned char* input_buffer_t;

// Task human interface
/// Display a text representation of a task on the terminal

/// \param task Task to be displayed
int
pelib_printf(task_tp)(task_tp task);
int
pelib_printf_detail(task_tp)(task_tp task, int);
/// Allocates and obtains a string representation of a task

/// The pointer receiving the result must *not* be initialized and given in second argument *and* receive the result of the function
char *
pelib_string(task_tp)(task_tp);
char *
pelib_string_detail(task_tp)(task_tp, int);
/// Compare one tasks to another
/* Returns 0 is both tasks are identical, an other value otherwise.
 */
int	
pelib_compare(task_tp)(task_tp, task_tp);
// Task manipulation
/// Initializes a task

/// Does *not* allocate memory before hand
task_tp*
pelib_alloc(task_tp)(void*);

size_t
pelib_fread(task_tp)(task_tp*, size_t, size_t, FILE*);
int
pelib_init(task_tp)(task_tp*);
#endif

#endif
