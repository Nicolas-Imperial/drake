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

#if DEBUG
/* get REG_EIP from ucontext.h */
//#define __USE_GNU
#include <ucontext.h>
#endif
#include <execinfo.h>

#include <pelib/integer.h>
#include <pelib/string.h>

#include <drake/task.h>
#include <drake/node.h>
#include <drake/link.h>
#include <drake/cross_link.h>
#include <drake/processor.h>
#include <drake/mapping.h>
#include <drake/platform.h>
#include <drake/stream.h>
#include <pelib/monitor.h>
#include <drake/platform.h>
#include <drake/application.h>
#include <drake/scheduler.h>

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
	char **const_strings;
	size_t i;

	size = backtrace (array, 10);
	const_strings = backtrace_symbols (array, size);

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
		printf("%s ", const_strings[i]);
	}
	printf("\n");
	}

	abort();
}
#endif

static
void
init_schedule(drake_exec_task_t *start, unsigned int number_of_task_instances)
{
	// Reinitialize schedule
	unsigned int i;
	drake_exec_task_t *schedule = start;
	for(i = 0; i < number_of_task_instances - 1; i++)
	{
		schedule[i].next = &schedule[i + 1];
		schedule[i].round_next = &schedule[i + 1];
	}
	schedule[i].next = NULL;
	schedule[i].round_next = NULL;
}


drake_stream_t
drake_stream_create_explicit(drake_application_t*(*get_app)(), drake_platform_t pt)
{
	// Get application data
	drake_stream_t stream;
	stream.application = get_app();
	stream.platform = pt;

	// Do run init
	debug("Application create");
	unsigned int i;

// Potential inconsistency checks
// Private memory allocated queues between tasks mapped to different cores
// Private memory allocated queues between tasks running on at least 2 cores

	// First allocate all shared queues
#if 0
	for(i = 0; i < stream.application->number_of_cores; i++)
	{
		drake_core_shared_link_t ii = stream.application->core_shared_queue[i];

		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_shared_queue_allocation_t jj = ii.allocation[j];
			jj.link->queue = (cfifo_t(char)*)drake_platform_malloc(sizeof(cfifo_t(char)), i, jj.link->type, jj.link->level);
			cfifo_t(char) *fifo = jj.link->queue;
			fifo->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(i, jj.link->type, jj.link->level), fifo->capacity, i, jj.link->type, jj.link->level);
			fifo->capacity = jj.capacity;
			pelib_init(cfifo_t(char))(fifo);
			debug("Initializing shared queue");
		}
	}
#endif
	
	// Allocate all private link buffers
	// Actual link already statically allocated; just need to allocate their buffer
	// for private memory
	i = drake_platform_core_id();
	//for(i = 0; i < sizeof(core_private_link) / sizeof(drake_abstract_link_t*); i++)
	{
		drake_core_private_link_t ii = stream.application->core_private_link[i];
		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_abstract_link_t *jj = &ii.link[j];
			jj->queue->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(jj->core, jj->type, jj->level), jj->queue->capacity, jj->core, jj->type, jj->level);
			pelib_init(cfifo_t(char))(jj->queue);
			debug("Initializing private queue");
		}
	}

	// Allocating distributed input queues
	for(i = 0; i < stream.application->number_of_cores; i++)
	{
		drake_core_input_distributed_link_t ii = stream.application->core_input_distributed_link[i];

		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_input_distributed_link_t *jj = &ii.link[j];
			cfifo_t(char) *fifo = jj->link->queue;
			fifo->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(i, jj->link->type, jj->link->level), fifo->capacity, i, jj->link->type, jj->link->level);
			jj->read = (size_t*)drake_platform_malloc(sizeof(size_t), i, jj->link->type, jj->link->level);
			jj->state = (drake_task_state_t*)drake_platform_malloc(sizeof(drake_task_state_t), i, jj->link->type, jj->link->level);
			pelib_init(cfifo_t(char))(fifo);
			debug("Initializing distributed input queue");
		}
	}
	
	// Allocating distributed output queues
	for(i = 0; i < stream.application->number_of_cores; i++)
	{
		drake_core_output_distributed_link_t ii = stream.application->core_output_distributed_link[i];

		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_output_distributed_link_t *jj = &ii.link[j];
			cfifo_t(char) *fifo = jj->link->queue;
			fifo->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(i, jj->link->type, jj->link->level), fifo->capacity, i, jj->link->type, jj->link->level);
			jj->write = (size_t*)drake_platform_malloc(sizeof(size_t), i, jj->link->type, jj->link->level);
			pelib_init(cfifo_t(char))(fifo);
			debug("Initializing distributed output queue");
		}
	}

	return stream;
}

int
drake_stream_init(drake_stream_t *stream, void *aux)
{
	int success;

	init_schedule(stream->application->schedule[drake_platform_core_id()], stream->application->number_of_task_instances[drake_platform_core_id()]);

	// This will come when the application is initialized
	// By now it should be possible to initialize all tasks
	unsigned int core = drake_platform_core_id();
	drake_exec_task_t *exec_task;
	for(exec_task = stream->application->schedule[0]; exec_task != NULL; exec_task = exec_task->next) 
	{
		drake_task_t *task = exec_task->task;
		if(task->state == DRAKE_TASK_STATE_INIT)
		{
			success = success && task->init(aux);
			task->state = DRAKE_TASK_STATE_START;
		}
	}

	drake_platform_barrier(NULL);
	return success;
}

int
drake_stream_destroy(drake_stream_t* stream)
{
	int success;
	init_schedule(stream->application->schedule[drake_platform_core_id()], stream->application->number_of_task_instances[drake_platform_core_id()]);

	// This will come when the application is initialized
	// By now it should be possible to initialize all tasks
	unsigned int core = drake_platform_core_id();
	drake_exec_task_t *exec_task;
	for(exec_task = stream->application->schedule[drake_platform_core_id()]; exec_task != NULL; exec_task = exec_task->next) 
	{
		drake_task_t *task = exec_task->task;
		if(task->state < DRAKE_TASK_STATE_DESTROY)
		{
			success = success && task->destroy();
			task->state = DRAKE_TASK_STATE_DESTROY;
		}
	}

	return success;
}

int
drake_stream_run(drake_stream_t* stream)
{
	return drake_dynamic_scheduler(stream);
}

