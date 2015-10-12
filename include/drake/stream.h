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
#include <drake/mapping.h>
#include <drake/processor.h>
#include <drake/platform.h>
#include <drake/schedule.h>

#ifndef DRAKE_STREAM_H
#define DRAKE_STREAM_H

/** Drake stream application **/
typedef struct {
	/// Mapping information
	mapping_t *mapping;
	/// List of processors in the application
	processor_t *proc;
	/// Size in bytes of the active core's local communication memory
	size_t local_memory_size;
	/// Time of the current pipeline stage, calculated from its start and stop time
	drake_time_t stage_time;
	/// Time at which the active core started the last pipeline stage
	drake_time_t stage_start_time;
	/// Time at which the active core stopped the last pipeline stage
	drake_time_t stage_stop_time;
	/// Time the active core needs to sleep in milliseconds before the next pipeline stage
	drake_time_t stage_sleep_time;
	/// Function pointer to cleanup and free the memory associated to the scheduling information of the application
	void (*schedule_destroy)();
	/// Schedule of the streaming application
	drake_schedule_t schedule;
	/// Function pointer to the function that returns the function pointer corresponding to a task and its state
	void* (*func)(size_t id, task_status_t status);
	/// Contains all management data relative to the platform that runs the stream
	drake_platform_t platform;
	/// Representation of time 0 depending on platform implementation
	drake_time_t zero;
} drake_stream_t;

/** Create a streaming application using schedule information from functions given as arguments
	@param schedule_init Function that initialises data structure that holds scheduling information
	@param schedule_destroy Function that cleans up a schedule and frees the associated memory
	@param task_function Function that returns the function pointer corresponding to a task and its state
**/
drake_stream_t drake_stream_create_explicit(void (*schedule_init)(), void (*schedule_destroy)(drake_schedule_t*), void* (*task_function)(size_t id, task_status_t status));
/** Initialises a stream already created. Runs the init() method of each of its tasks
	@param stream Stream to be initialized
	@param arg Memory address that holds arguments to transmit to task initialization function
**/
int drake_stream_init(drake_stream_t* stream, void* arg);
/** Run the streaming application **/
int drake_stream_run(drake_stream_t*);
/** Destroys the stream and frees its associated memory **/
int drake_stream_destroy(drake_stream_t*);

#endif
