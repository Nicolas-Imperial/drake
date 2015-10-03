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


#include <stddef.h>

#ifndef TASK_H
#define TASK_H
/**  Possible state of a task **/
enum task_status {TASK_INVALID, TASK_INIT, TASK_START, TASK_RUN, TASK_KILLED, TASK_ZOMBIE, TASK_DESTROY};
/** Space-less alias for enum_task_status **/
typedef enum task_status task_status_t;

/** Type of a simple task id **/
typedef unsigned int task_id;

/** Forward declaration of a link betwen tasks **/
struct link;
/** Space-less type alias for struct link **/
typedef struct link link_t;
/** Symbol-less type alias for pointer to link **/
typedef link_t* link_tp;

/** Generate pelib objects from link structure **/
#define STRUCT_T link_t
#include <pelib/structure.h>
#define DONE_link_t 1

/** Generate pelib objects from pointer to link **/
#define STRUCT_T link_tp
#include <pelib/structure.h>
#define DONE_link_tp 1

/** Defines arrays for pointers to link **/
#define ARRAY_T link_tp
#include <pelib/array.h>
#define DONE_array_link_tp 1

/** Forward declarations for cross links **/
struct cross_link;
/** Space-less type alias for struct cross **/
typedef struct cross_link cross_link_t;
/** Symbol-less type alias for pointer to cross-link **/
typedef cross_link_t* cross_link_tp;

/** Generate pelib objects from cross-link structure **/
#define STRUCT_T cross_link_t
#include <pelib/structure.h>
#define DONE_cross_link_t 1

/** Generate pelib objects from pointer to cross link **/
#define STRUCT_T cross_link_tp
#include <pelib/structure.h>
#define DONE_cross_link_tp 1

/** Defines arrays for pointers to cross link **/
#define ARRAY_T cross_link_tp
#include <pelib/array.h>
#define DONE_array_cross_link_tp 1

/** Forward declarations for processors **/
struct processor;
/** Space-less type alias for struct processor **/
typedef struct processor processor_t;

/** Model of a streaming task **/
struct task
{
	/// Identification of the task. All tasks (say n tasks) should be numbered continuously from 1 to n to be friendly with files generated to ILP solver and task graph graphic generator. 
	task_id id;
 	/// Pointer to the processor structure this task is mapped to
 	processor_t* core;
	/// List of link pointers that consume data produced by this task
	array_t(link_tp) *succ;
 	/// List of link pointers that produce data this task consumes
 	array_t(link_tp) *pred;
	/// List of links toward consumer tasks mapped to another core
	array_t(cross_link_tp) *sink;
 	/// List of links toward producer tasks mapped to another cores
 	array_t(cross_link_tp) *source;
	/// Frequency in KHz this task should run at
	int frequency;
 	/// State of the task: initialized, running, killed, etc.
 	task_status_t status;
	/// Human-readable identifier for the task
	char *name;
 	/// Pointer to initialisation function, run before the stream begins
 	int (*init)(struct task*, void*);
	/// Pointer to function to run when tasks begins to work, run after the stream begins
	int (*start)(struct task*);
 	/// Pointer to task's main work function
 	int (*run)(struct task*);
	/// Pointer to function when the task is killed, while the stream still runs
	int (*kill)(struct task*);
 	/// Pointer to dunction when the task is destroyed, run after the stream terminates
 	int (*destroy)(struct task*);
	/// Monitoring variables
	unsigned long long int start_time, stop_time, start_presort, stop_presort, check_time, push_time, work_time, check_errors, check_recv, check_putback, check_feedback, put_reset, put_pop, put_send, check_wait, push_wait, work_wait, work_read, work_write;
 	/// Monitoring variables
 	unsigned long long int step_init, step_start, step_check, step_work, step_push, step_killed, step_zombie, step_transition;
};
/// Space-less type alias for struct task
typedef struct task task_t;
/// Symbol-less type alias for pointer to task_t
typedef struct task* task_tp;

/** Generate pelib objects from task structure **/
#define STRUCT_T task_t
#include <pelib/structure.h>
#define DONE_task_t 1

/** Generate pelib objects from pointer to task structure **/
#define STRUCT_T task_tp
#include <pelib/structure.h>
#define DONE_task_tp 1

/** Defines arrays for pointers to tasks **/
#define ARRAY_T task_tp
#include <pelib/array.h>
#define DONE_array_task_tp 1

/** Return 0 of there is at least one element left in any input link of the task, or if at least one producer is still working. Return the value computed by this function in the work function of a task to terminate it as soon as it has no more data to process. **/
int
drake_task_depleted(task_tp task);

#endif
