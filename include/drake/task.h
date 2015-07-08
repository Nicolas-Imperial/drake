#include <stddef.h>

#ifndef TASK_H
#define TASK_H

enum task_status {TASK_INVALID, TASK_INIT, TASK_START, TASK_RUN, TASK_KILLED, TASK_ZOMBIE};
typedef enum task_status task_status_t;

typedef unsigned int task_id;

// Forward declarations for links
struct link;
typedef struct link link_t;
typedef link_t* link_tp;

#define STRUCT_T link_t
#include <pelib/structure.h>
#define DONE_link_t 1

#define STRUCT_T link_tp
#include <pelib/structure.h>
#define DONE_link_tp 1

#define ARRAY_T link_tp
#include <pelib/array.h>
#define DONE_array_link_tp 1

// Forward declarations for cross links
struct cross_link;
typedef struct cross_link cross_link_t;
typedef cross_link_t* cross_link_tp;

#define STRUCT_T cross_link_t
#include <pelib/structure.h>
#define DONE_cross_link_t 1

#define STRUCT_T cross_link_tp
#include <pelib/structure.h>
#define DONE_cross_link_tp 1

#define ARRAY_T cross_link_tp
#include <pelib/array.h>
#define DONE_array_cross_link_tp 1

struct processor;
typedef struct processor processor_t;

struct task;
typedef struct task task_t;

struct task
{
	/// Identification of the task. All tasks (say n tasks) should be numbered continuously from 1 to n to be friendly with files generated to ILP solver and task graph graphic generator.
	task_id id;
	processor_t* core;
	array_t(link_tp) *pred, *succ;
	array_t(cross_link_tp) *sink;
	array_t(cross_link_tp) *source;
	int frequency;
	task_status_t status;
	int (*init)(task_t*, void*);
	int (*start)(task_t*);
	int (*run)(task_t*);
	int (*destroy)(task_t*);
	unsigned long long int start_time, stop_time, start_presort, stop_presort, check_time, push_time, work_time, check_errors, check_recv, check_putback, check_feedback, put_reset, put_pop, put_send, check_wait, push_wait, work_wait, work_read, work_write;
	unsigned long long int step_init, step_start, step_check, step_work, step_push, step_killed, step_zombie, step_transition;
};
typedef task_t* task_tp;

#define STRUCT_T task_t
#include <pelib/structure.h>
#define DONE_task_t 1

#define STRUCT_T task_tp
#include <pelib/structure.h>
#define DONE_task_tp 1

#define ARRAY_T task_tp
#include <pelib/array.h>
#define DONE_array_task_tp 1

#endif
