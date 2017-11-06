#include <drake/scheduler.h>
#include <drake/platform.h>
#include <drake/task.h>
#include <drake/application.h>

#if 1
#define debug(var) printf("[%s:%s:%d:P%u] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:P%u] %s = \"%p\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:P%u] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_uint(var) printf("[%s:%s:%d:P%u] %s = \"%u\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_luint(var) printf("[%s:%s:%d:P%u] %s = \"%lu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:P%u] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

int
drake_dynamic_scheduler(drake_stream_t* stream)
{
	unsigned int core = drake_platform_core_id();
	unsigned int i;
	unsigned int success = 1;
	drake_exec_task_t root;
	root.next = stream->application->schedule[core];
	root.round_next = root.next;
	
	while(root.next != NULL) // While root still has a task
	{
		drake_exec_task_t *exec_task = root.next;
		drake_exec_task_t *last_task = exec_task;

		while(exec_task != NULL)
		{
			drake_task_t *task = exec_task->task;
			task->instance = exec_task;
			if(task->asleep == 0)
			{
				switch(task->state)
				{
					case DRAKE_TASK_STATE_START:
						if(drake_platform_core_id() == exec_task->master_core)
						{
							*exec_task->return_value = task->start();
							if(exec_task->width > 1)
							{
								drake_task_pool_destroy(exec_task->pool);
							}
						}
						else
						{
							// Get stuck in thread pool
							drake_task_pool_create(exec_task->pool);
						}

						if(*exec_task->return_value)
						{
							if(last_task == exec_task) // This is the first task in round
							{
								root.round_next = exec_task->round_next;
							}
							else
							{
								last_task->round_next = exec_task->round_next;
							}
							task->state = DRAKE_TASK_STATE_RUN;
							task->asleep = 1;
						}
					break;
					case DRAKE_TASK_STATE_RUN:
					{
						unsigned int core_id = drake_platform_core_id();
						int test = core_id == exec_task->master_core;
						if(test)
						{
							*exec_task->return_value = task->run();
							if(exec_task->width > 1)
							{
								drake_task_pool_destroy(exec_task->pool);
							}
						}
						else
						{
							drake_task_pool_create(exec_task->pool);
						}

						if(*exec_task->return_value & (DRAKE_TASK_CONDITION_SLEEP | DRAKE_TASK_CONDITION_KILL))
						{
							// Take task out of next round schedule
							if(last_task == exec_task) // This is the first task in round
							{
								root.round_next = exec_task->round_next;
							}
							else
							{
								last_task->round_next = exec_task->round_next;
							}
							task->asleep = 1;
						}
						else
						{
						}
						if(*exec_task->return_value & DRAKE_TASK_CONDITION_KILL)
						{
							task->state = DRAKE_TASK_STATE_KILL;
						}
						break;
					}
				}
				task->instance = NULL;
			}
			else
			{
				// Remove task instance from round schedule
				if(last_task == exec_task) // This is the first task in round
				{
					root.round_next = exec_task->round_next;
				}
				else
				{
					last_task->round_next = exec_task->round_next;
				}

			}

			// Same previous task and get to next task
			last_task = exec_task;
			exec_task = exec_task->round_next;
		}

		// Restore pipeline and turn all task state into RUN
		exec_task = root.next;
		last_task = exec_task;
		while(exec_task != NULL)
		{
			drake_task_t *task = exec_task->task;
			if(exec_task->task->asleep != 0)
			{
				exec_task->task->asleep = 0;
			}

			// If task is killed, then also remove from next round
			if(task->state >= DRAKE_TASK_STATE_KILL)
			{
				if(task->state == DRAKE_TASK_STATE_KILL)
				{
					task->instance = exec_task;
					unsigned int core_id = drake_platform_core_id();
					int test = core_id == exec_task->master_core;
					if(test)
					{
						*exec_task->return_value = task->kill();
						if(exec_task->width > 1)
						{
							drake_task_pool_destroy(exec_task->pool);
						}
					}
					else
					{
						drake_task_pool_create(exec_task->pool);
					}
					success = success && *exec_task->return_value;
					task->instance = NULL;
				}

				if(last_task == exec_task) // This is the first task in round
				{
					root.next = exec_task->next;
				}
				else
				{
					last_task->next = exec_task->next;
				}
				task->state = DRAKE_TASK_STATE_ZOMBIE;
			}

			last_task = exec_task;
			exec_task->round_next = exec_task->next;
			exec_task = exec_task->next;
		}
	}

	return success;
}

