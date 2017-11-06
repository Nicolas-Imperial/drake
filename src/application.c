#include <drake/application.h>

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


size_t
drake_buffer_capacity(drake_abstract_link_t *link)
{
	/*
	debug_size_t(link->queue->read);
	debug_size_t(link->queue->write);
	debug_size_t(pelib_cfifo_capacity(char)(link->queue));
	debug_size_t(pelib_cfifo_length(char)(link->queue));
	debug_addr(link->queue->buffer);
	*/
	return pelib_cfifo_capacity(char)(link->queue) / link->token_size;
}

size_t
drake_buffer_free(drake_abstract_link_t *link)
{
	return (pelib_cfifo_capacity(char)(link->queue) - pelib_cfifo_length(char)(link->queue) ) / link->token_size;
}

size_t
drake_buffer_available(drake_abstract_link_t *link)
{
	return pelib_cfifo_length(char)(link->queue) / link->token_size;
}

size_t
drake_buffer_free_continuous(drake_abstract_link_t *link)
{
	size_t size;
	pelib_cfifo_writeaddr(char)(link->queue, &size, NULL);
	return size / link->token_size;
}

size_t
drake_buffer_available_continuous(drake_abstract_link_t *link)
{
	size_t size;
	pelib_cfifo_peekaddr(char)(link->queue, 0, &size, NULL);
	return size / link->token_size;
}

void*
drake_buffer_freeaddr(drake_abstract_link_t *link, size_t *size, void** extra)
{
	void* addr = (void*)pelib_cfifo_writeaddr(char)(link->queue, size, (char**)extra);
 	*size /= link->token_size;
	return addr;
}

void*
drake_buffer_availableaddr(drake_abstract_link_t *link, size_t skip, size_t *size, void** extra)
{
	void* addr = (void*)pelib_cfifo_peekaddr(char)(link->queue, skip, size, (char**)extra);
 	*size /= link->token_size;
	return addr;
}

void
drake_buffer_commit(drake_abstract_link_t *link, size_t size)
{
	pelib_cfifo_fill(char)(link->queue, size * link->token_size);
}

void
drake_buffer_discard(drake_abstract_link_t *link, size_t size)
{
	pelib_cfifo_discard(char)(link->queue, size * link->token_size);
}

int
drake_task_kill(int condition)
{
	return (condition != 0) << (DRAKE_TASK_CONDITION_KILL >> 1);
}

int
drake_task_sleep(int condition)
{
	return (condition != 0) << (DRAKE_TASK_CONDITION_SLEEP >> 1);
}

unsigned int
drake_task_get_width(drake_task_tp task)
{
	return task->instance->width; 
}

const char*
drake_task_get_name(drake_task_tp task)
{
	return task->name;
}

int drake_buffer_depleted(drake_abstract_link_t *link)
{
	return link->producer->state >= DRAKE_TASK_STATE_KILL && pelib_cfifo_length(char)(link->queue) == 0;
}

int
drake_autokill_task(drake_task_tp task)
{
	int all_empty = 1;
	int all_killed = 1;
	size_t i;
	for(i = 0; i < task->number_of_input_links; i++)
	{
		all_empty = all_empty && pelib_cfifo_length(char)(task->input_link[i]->queue) == 0;
		all_killed = all_killed && task->input_link[i]->producer->state >= DRAKE_TASK_STATE_KILL;
	}

	return (all_empty && all_killed) << (DRAKE_TASK_CONDITION_KILL >> 1);
}

int
drake_autosleep_task(drake_task_tp task)
{
	int all_sleeping = 1;
	int all_empty = 1;

	size_t i;
	for(i = 0; i < task->number_of_input_links; i++)
	{
		all_empty = all_empty && pelib_cfifo_length(char)(task->input_link[i]->queue) == 0;
		all_sleeping = all_sleeping && task->input_link[i]->producer->asleep != 0;
	}

	return (all_empty && all_sleeping) << (DRAKE_TASK_CONDITION_SLEEP >> 1);
}

int
drake_autoexit_task(drake_task_tp task)
{
	int sleep = drake_autosleep_task(task);
	int kill = drake_autokill_task(task);
	return sleep | kill;
}

