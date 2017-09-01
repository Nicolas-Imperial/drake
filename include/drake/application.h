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


#include <pelib/char.h>
#include <drake/platform.h>

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef DRAKE_APPLICATION_H
#define DRAKE_APPLICATION_H

#define DRAKE_TASK_CONDITION_KILL 1
#define DRAKE_TASK_CONDITION_SLEEP 2

struct drake_abstract_link;
struct drake_input_distributed_link;
struct drake_output_distributed_link;
typedef enum { DRAKE_TASK_STATE_INIT, DRAKE_TASK_STATE_START, DRAKE_TASK_STATE_RUN, DRAKE_TASK_STATE_KILL, DRAKE_TASK_STATE_ZOMBIE, DRAKE_TASK_STATE_DESTROY } drake_task_state_t;
struct drake_exec_task;

struct drake_task
{
	int (*init)(void*);
	int (*start)();
	int (*run)();
	int (*kill)();
	int (*destroy)();
	int asleep;
	drake_task_state_t state;
	const char *name;
	struct drake_exec_task *instance;
	size_t number_of_input_links;
	size_t number_of_output_links;
	size_t number_of_input_distributed_links;
	size_t number_of_output_distributed_links;
	struct drake_abstract_link **input_link, **output_link;
	struct drake_input_distributed_link *input_distributed_link;
	struct drake_output_distributed_link *output_distributed_link;
};
typedef struct drake_task drake_task_t;
typedef struct drake_task *drake_task_tp;

struct drake_abstract_link
{
	// Application structure and data buffers
	drake_task_t *producer, *consumer;
	cfifo_t(char) *queue;
	size_t token_size;
	// Allocation details
	unsigned int core;
	drake_memory_t type;
	unsigned int level;
};
typedef struct drake_abstract_link drake_abstract_link_t;

struct drake_core_private_link
{
	unsigned int number_of_links;
	struct drake_abstract_link *link;
};
typedef struct drake_core_private_link drake_core_private_link_t;

struct drake_input_distributed_link
{
	struct drake_abstract_link *link;
	size_t *read;
	drake_task_state_t *state;
	unsigned int core;
	drake_memory_t type;
	unsigned int level;
};
typedef struct drake_input_distributed_link drake_input_distributed_link_t;

struct drake_core_input_distributed_link
{
	unsigned int number_of_links;
	struct drake_input_distributed_link *link;
};
typedef struct drake_core_input_distributed_link drake_core_input_distributed_link_t;

struct drake_output_distributed_link
{
	struct drake_abstract_link *link;
	size_t *write;
	unsigned int core;
	drake_memory_t type;
	unsigned int level;
};
typedef struct drake_output_distributed_link drake_output_distributed_link_t;

struct drake_core_output_distributed_link
{
	unsigned int number_of_links;
	struct drake_output_distributed_link *link;
};
typedef struct drake_core_output_distributed_link drake_core_output_distributed_link_t;

struct drake_exec_task
{
	drake_task_t *task;
	double start_time;
	unsigned int width;
	unsigned int frequency;
	struct drake_exec_task *next, *round_next;
};
typedef struct drake_exec_task drake_exec_task_t;

struct drake_application
{
	unsigned int number_of_cores;
	unsigned int number_of_tasks;
	unsigned int *number_of_task_instances;
	drake_task_t *task;
	drake_core_private_link_t *core_private_link;
	drake_core_input_distributed_link_t *core_input_distributed_link;
	drake_core_output_distributed_link_t *core_output_distributed_link;
	drake_exec_task_t **schedule;
};
typedef struct drake_application drake_application_t;

size_t drake_task_get_width(drake_task_tp task);
const char* drake_task_get_name(drake_task_tp task);
int drake_autokill_task(drake_task_tp);
int drake_autosleep_task(drake_task_tp);
int drake_autoexit_task(drake_task_tp);
int drake_task_kill(int condition);
int drake_task_sleep(int condition);

size_t drake_buffer_capacity(drake_abstract_link_t *link);
size_t drake_buffer_free(drake_abstract_link_t *link);
size_t drake_buffer_available(drake_abstract_link_t *link);
size_t drake_buffer_free_continuous(drake_abstract_link_t *link);
size_t drake_buffer_available_continuous(drake_abstract_link_t *link);
void* drake_buffer_freeaddr(drake_abstract_link_t *link, size_t *size, void** extra);
void* drake_buffer_availableaddr(drake_abstract_link_t *link, size_t skip, size_t *size, void** extra);
void drake_buffer_commit(drake_abstract_link_t *link, size_t size);
void drake_buffer_discard(drake_abstract_link_t *link, size_t size);

#endif

#if defined APPLICATION && PELIB_CONCAT_2(DONE_, APPLICATION) == 0
#define drake_application_create PELIB_##CONCAT_2(drake_application_create_, APPLICATION)
#define drake_application_init PELIB_##CONCAT_2(drake_application_init_, APPLICATION)
#define drake_application_run PELIB_##CONCAT_2(drake_application_run_, APPLICATION)
#define drake_application_destroy PELIB_##CONCAT_2(drake_application_destroy_, APPLICATION)
#define drake_application_build PELIB_##CONCAT_2(drake_application_build_, APPLICATION)
#define drake_application_number_of_tasks PELIB_##CONCAT_2(drake_application_number_of_tasks_, APPLICATION)
/** Returns the core id of a core within a parallel task **/
unsigned int drake_application_number_of_tasks();
struct drake_application* drake_application_build();
#else
#define drake_application_create(application) PELIB_##CONCAT_2(drake_application_create_, application)
#define drake_application_init(application) PELIB_##CONCAT_2(drake_application_init_, application)
#define drake_application_run(application) PELIB_##CONCAT_2(drake_application_run_, application)
#define drake_application_destroy(application) PELIB_##CONCAT_2(drake_application_destroy_, application)
#define drake_application_build(application) PELIB_##CONCAT_2(drake_application_build_, application)
#define drake_application_number_of_tasks(application) PELIB_##CONCAT_2(drake_application_number_of_tasks_, application)
struct drake_application* drake_application_build(application)();
#endif

#ifdef __cplusplus
}
#endif

