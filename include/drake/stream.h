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

#define drake_stream_create(application) drake_stream_create_explicit(PELIB_##CONCAT_2(drake_schedule_init_, application), PELIB_##CONCAT_2(drake_schedule_destroy_, application), PELIB_CONCAT_2(drake_function_, application))

typedef struct {
	void* base_ptr;
	size_t size;
	size_t* stack_ptr;
} drake_stack_t;

typedef struct {
	mapping_t *mapping;
	processor_t *proc;
	drake_stack_t *stack;
	size_t local_memory_size;
	drake_time_t stage_time, stage_start_time, stage_stop_time, stage_sleep_time;
	void (*schedule_destroy)();
	drake_schedule_t schedule;
	void* (*func)(size_t id, task_status_t status);
} drake_stream_t;

drake_stream_t drake_stream_create_explicit(void (*schedule_init)(), void (*schedule_destroy)(), void* (*task_function)(size_t id, task_status_t status));
int drake_stream_init(drake_stream_t*, void*);
int drake_stream_run(drake_stream_t*);
void drake_stream_destroy(drake_stream_t*);

#endif
