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


#include <drake/task.h>


#ifndef DRAKE_SCHEDULE_H
#define DRAKE_SCHEDULE_H

/** Scheduling information of a task in a pelib stream **/
typedef struct {
	/// Numeric id of a task within Drake framework
	size_t id;
	/// Time in millisecond the task should start at
	double start_time;
	/// Frequency in KHz the task should run at
	double frequency;
} drake_schedule_task_t;

/** Scheduling information of a drake streaming application **/
typedef struct {
	/// Number of cores in the schedule
	size_t core_number;
	/// Number of tasks in the application
	size_t task_number;
	/// String id as in taskgraph and schedule of a task
	char **task_name;
	double *task_workload;
	/// Time in millisecond of a pipeline stage
	double stage_time;
	/// Number of tasks mapped to a core
	size_t *tasks_in_core;
	/// Number of tasks mapped to a core that have at least one producer mapped to another core
	size_t *consumers_in_core;
	/// Number of tasks mapping to a core that have at least one consumer mapped to another core
	size_t *producers_in_core;
	/// Number of consumers of a task
	size_t *consumers_in_task;
	/// Number of producers of a task
	size_t *producers_in_task;
	/// For each core, list of task ids that consume data produced by tasks mapped to this core
	size_t **consumers_id;
	/// Name of output channels as seen by consumer task
	char ***consumers_name;
	/// For each core, list of task ids that procure data consumed by tasks mapped to this core
	size_t **producers_id;
	/// Data production rate in SDF context
	size_t **producers_rate;
	/// Data consumption rate in SDF context
	size_t **consumers_rate;
	/// Name of output channels for a task
	char ***output_name;
	/// Name of input channels for a task
	char ***input_name;
	/// Name of input channels as seen by the producer task
	char ***producers_name;
	/// Number of consumers of a task that are mapped to another core than this task is mapped to
	size_t *remote_consumers_in_task;
	/// Number of producers of a task that are mapped to another core than this task is mapped to
	size_t *remote_producers_in_task;
	/// Scheduling information for each task in the streaming application
	drake_schedule_task_t **schedule;
} drake_schedule_t;

#endif

#if defined APPLICATION && PELIB_CONCAT_2(DONE_, APPLICATION) == 0

#define drake_function PELIB_##CONCAT_2(drake_function_, APPLICATION)
#define drake_schedule_init PELIB_##CONCAT_2(drake_schedule_init_, APPLICATION)
#define drake_schedule_destroy PELIB_##CONCAT_2(drake_schedule_destroy_, APPLICATION)
#define drake_task_number PELIB_##CONCAT_2(drake_task_number_, APPLICATION)
#define drake_task_name PELIB_##CONCAT_2(drake_task_name_, APPLICATION)
/** Returns the core id of a core within a parallel task **/
size_t drake_core_id(task_tp task);
size_t drake_task_width(task_tp task);
/** Returns the function pointer that corresponds to a task in a certain state **/
void* drake_function(size_t id, task_status_t state);
/** Initialize scheduling information for the streaming application **/
void drake_schedule_init(drake_schedule_t*);
/** Cleans up scheduling information when the stream is destroyed **/
void drake_schedule_destroy(drake_schedule_t*);
/** Returns the number of tasks in a streaming application **/
int drake_task_number();
/** Returns the string description of a task from its id defined in drake framework **/
char* drake_task_name(size_t);
#else
#define drake_function(application) PELIB_##CONCAT_2(drake_function_, application)
#define drake_schedule_init(application) PELIB_##CONCAT_2(drake_schedule_init_, application)
#define drake_schedule_destroy(application) PELIB_##CONCAT_2(drake_schedule_destroy_, application)
#define drake_task_number(application) PELIB_##CONCAT_2(drake_task_number_, application)
#define drake_task_name(application) PELIB_##CONCAT_2(drake_task_name_, application)
#endif

