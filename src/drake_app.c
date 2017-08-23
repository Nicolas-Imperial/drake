#include <stdio.h>
#include <stdlib.h>

#include <drake/schedule.h>
#include <pelib/char.h>
#include <drake/platform.h>
#include <drake/node.h>

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
	struct drake_abstract_link **input_link, **output_link;
	struct drake_input_distributed_link *input_distributed_link;
	struct drake_output_distributed_link *output_distributed_link;
};
typedef struct drake_task drake_task_t;

struct drake_abstract_link
{
	// Application structure and data buffers
	drake_task_t *producer, *consumer;
	cfifo_t(char) *queue;
	size_t token_size;
	// Allocation details
	drake_memory_t type;
	unsigned int core_or_island;
	unsigned int level;
};
typedef struct drake_abstract_link drake_abstract_link_t;

struct drake_core_private_link
{
	unsigned int number_of_links;
	struct drake_abstract_link **link;
};
typedef struct drake_core_private_link drake_core_private_link_t;

struct drake_input_distributed_link
{
	struct drake_abstract_link *link;
	size_t *read;
	drake_task_state_t *state;
};
typedef drake_input_distributed_link drake_input_distributed_link_t;

struct drake_core_input_distributed_link
{
	unsigned int number_of_links;
	struct drake_input_distributed_link **link;
};
typedef struct drake_core_input_distributed_link drake_core_input_distributed_link_t;

struct drake_output_distributed_link
{
	struct drake_abstract_link *link;
	size_t *write;
};
typedef drake_output_distributed_link drake_output_distributed_link_t;

struct drake_core_output_distributed_link
{
	unsigned int number_of_links;
	struct drake_output_distributed_link **link;
};
typedef drake_core_output_distributed_link drake_core_output_distributed_link_t;

struct drake_shared_queue_allocation
{
	size_t capacity;
	struct drake_abstract_link *link;
};
typedef struct drake_shared_queue_allocation drake_shared_queue_allocation_t;

struct drake_core_shared_link
{
	unsigned int number_of_links;
	struct drake_shared_queue_allocation *allocation;
};
typedef struct drake_core_shared_link drake_core_shared_link_t;

struct drake_exec_task
{
	drake_task_t *task;
	double start_time;
	unsigned int width;
	struct drake_exec_task *next, *round_next;
};
typedef struct drake_exec_task drake_exec_task_t;

struct schedule
{
	unsigned int number_of_tasks;
	unsigned int number_of_unique_tasks;
	struct drake_exec_task *task;
};
typedef struct schedule schedule_t;

// Forward task method declaration
int drake_init(hello, hello)(void*);
int drake_start(hello, hello)();
int drake_run(hello, hello)();
int drake_kill(hello, hello)();
int drake_destroy(hello, hello)();

int drake_init(world, world)(void*);
int drake_start(world, world)();
int drake_run(world, world)();
int drake_kill(world, world)();
int drake_destroy(world, world)();

// TODO: should we have it statically declared or allocated in fast private memory?
// Keep it static for now, assuming it will always be the most effective private memory
// Forward task declaration
extern drake_task_t task[2];
static drake_task_t *hello = &task[0];
static drake_task_t *world = &task[1];

// Fifos between tasks mapped to the same, unique processor and fifo between tasks mapped to core in different shared memory islands
static cfifo_t(char) hello_output_world_input_queue = { 0, 0, PELIB_CFIFO_POP, 14 * sizeof(char), NULL /* To be allocated at initialization */ };
// Fifos between tasks mapped to different cores in a unique island should be dynamically allocated to shared memory, as fifo between tasks run by more than 1 cores.
// Abstract links themselves are always private


// Links
static drake_abstract_link_t hello_output_world_input_link = { hello, world, &hello_output_world_input_queue, 1, (drake_memory_t)(DRAKE_MEMORY_SHARED | DRAKE_MEMORY_SMALL_CHEAP), 0, 0 };
static drake_abstract_link_t *hello_output = & hello_output_world_input_link;
static drake_abstract_link_t *world_input = & hello_output_world_input_link;

// Tasks
static drake_abstract_link_t *hello_input_link[] = {};
static drake_abstract_link_t *hello_output_link[] = { &hello_output_world_input_link };
static drake_input_distributed_link_t hello_input_distributed_link[] = {};
static drake_output_distributed_link_t hello_output_distributed_link[] = {};

static drake_abstract_link_t *world_input_link[] = { &hello_output_world_input_link };
static drake_abstract_link_t *world_output_link[] = {};
static drake_input_distributed_link_t world_input_distributed_link[] = {};
static drake_output_distributed_link_t world_output_distributed_link[] = {};

// Task collection
drake_task_t task[2] = {
	{ &drake_init(hello, hello), &drake_start(hello, hello), &drake_run(hello, hello), drake_kill(hello, hello), drake_destroy(hello, hello), 0, DRAKE_TASK_STATE_INIT, "hello", NULL, 0, 1, hello_input_link, hello_output_link, hello_input_distributed_link, hello_output_distributed_link },
	{ &drake_init(world, world), &drake_start(world, world), &drake_run(world, world), drake_kill(world, world), drake_destroy(world, world), 0, DRAKE_TASK_STATE_INIT, "world", NULL, 1, 0, world_input_link, world_output_link, world_input_distributed_link, world_output_distributed_link }
};

// Shared queues, pointer to their corresponding link and their capacity, sorted by owner processor
static drake_shared_queue_allocation_t core0_shared_queue_allocation[] = {};
static drake_core_shared_link_t core_shared_queue[] = { { 0, core0_shared_queue_allocation } };
// All links
static drake_abstract_link_t *core0_private_link[] = { &hello_output_world_input_link };
static drake_core_private_link_t core_private_link[] = { { 1, core0_private_link } };
static drake_core_input_distributed_link_t core_input_link[] = { { 0, {} } };
static drake_core_output_distributed_link_t core_output_link[] = { { 0, {} } };
// Schedule
static drake_exec_task_t core0_schedule[] = {
	{ hello, 0, 1, &core0_schedule[1], &core0_schedule[1] },
	{ world, 1, 1, &core0_schedule[2], &core0_schedule[2] },
	{ world, 2, 1, NULL, NULL }
};
drake_exec_task_t *schedule_start[] = { &core0_schedule[0] };

static size_t number_of_cores = 1;
static size_t number_of_tasks = 2;
static size_t number_of_task_instances[] = { 3 };

// Potential inconsistency checks
// Private memory allocated queues between tasks mapped to different cores
// Private memory allocated queues between tasks running on at least 2 cores

#ifdef __cplusplus
extern "C"
{
	int drake_application_create();
	int drake_application_init(void*);
	int drake_application_run();
	int drake_application_destroy();

}
#endif

size_t drake_task_get_width(drake_task_tp task)
{
	return task->instance->width; 
}

const char* drake_task_get_name(drake_task_tp task)
{
	return task->name;
}

drake_task_tp
drake_task(hello, hello)()
{
	return hello;
}

drake_task_tp
drake_task(world, world)()
{
	return world;
}

int
drake_application_create()
{
	debug("Application create");
	unsigned int i;

	// First allocate all shared queues
	for(i = 0; i < number_of_cores; i++)
	{
		drake_core_shared_link_t ii = core_shared_queue[i];

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
	
	// Allocate all private link buffers
	// Actual link already statically allocated; just need to allocate their buffer
	// for private memory
	i = drake_platform_core_id();
	//for(i = 0; i < sizeof(core_private_link) / sizeof(drake_abstract_link_t*); i++)
	{
		drake_core_private_link_t ii = core_private_link[i];
		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_abstract_link_t *jj = ii.link[j];
			jj->queue->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(jj->core_or_island, jj->type, jj->level), jj->queue->capacity, jj->core_or_island, jj->type, jj->level);
			pelib_init(cfifo_t(char))(jj->queue);
			debug("Initializing private queue");
		}
	}

	// Allocating distributed input queues
	for(i = 0; i < number_of_cores; i++)
	{
		drake_core_input_distributed_link_t ii = core_input_link[i];

		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_input_distributed_link_t *jj = ii.link[j];
			cfifo_t(char) *fifo = jj->link->queue;
			fifo->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(i, jj->link->type, jj->link->level), fifo->capacity, i, jj->link->type, jj->link->level);
			jj->read = (size_t*)drake_platform_malloc(sizeof(size_t), i, jj->link->type, jj->link->level);
			jj->state = (drake_task_state_t*)drake_platform_malloc(sizeof(drake_task_state_t), i, jj->link->type, jj->link->level);
			pelib_init(cfifo_t(char))(fifo);
			debug("Initializing distributed input queue");
		}
	}
	
	// Allocating distributed output queues
	for(i = 0; i < number_of_cores; i++)
	{
		drake_core_output_distributed_link_t ii = core_output_link[i];

		unsigned int j;
		for(j = 0; j < ii.number_of_links; j++)
		{
			drake_output_distributed_link_t *jj = ii.link[j];
			cfifo_t(char) *fifo = jj->link->queue;
			fifo->buffer = (char*)drake_platform_aligned_alloc(drake_platform_memory_alignment(i, jj->link->type, jj->link->level), fifo->capacity, i, jj->link->type, jj->link->level);
			jj->write = (size_t*)drake_platform_malloc(sizeof(size_t), i, jj->link->type, jj->link->level);
			pelib_init(cfifo_t(char))(fifo);
			debug("Initializing distributed output queue");
		}
	}
	return 1;
}

unsigned int
drake_application_number_of_tasks()
{
	return number_of_tasks;
}

size_t
drake_output_capacity(hello, hello, output)()
{
	return pelib_cfifo_capacity(char)(hello_output->queue) / hello_output->token_size;
}

size_t
drake_output_available(hello, hello, output)()
{
	return (pelib_cfifo_capacity(char)(hello_output->queue) - pelib_cfifo_length(char)(hello_output->queue) ) / hello_output->token_size;
}

size_t
drake_output_available_continuous(hello, hello, output)()
{
	size_t size;
	pelib_cfifo_peekaddr(char)(hello_output->queue, 0, &size, NULL);
	return size / hello_output->token_size;
}

char*
drake_output_buffer(hello, hello, output)(size_t *size, char **extra)
{
	char* addr = pelib_cfifo_writeaddr(char)(hello_output->queue, size, extra);
 	*size /= hello_output->token_size;
	return addr;
}

void
drake_output_commit(hello, hello, output)(size_t size)
{
	pelib_cfifo_fill(char)(hello_output->queue, size * hello_output->token_size);
}

size_t
drake_input_capacity(world, world, input)()
{
	return pelib_cfifo_capacity(char)(world_input->queue) / world_input->token_size;
}

size_t
drake_input_available(world, world, input)()
{
	return (pelib_cfifo_capacity(char)(world_input->queue) - pelib_cfifo_length(char)(world_input->queue) ) / world_input->token_size;
}

size_t
drake_input_available_continuous(world, world, input)()
{
	size_t size;
	pelib_cfifo_peekaddr(char)(world_input->queue, 0, &size, NULL);
	return size / world_input->token_size;
}

char*
drake_input_buffer(world, world, input)(size_t skip, size_t *size, char **extra)
{
	char* addr = pelib_cfifo_peekaddr(char)(world_input->queue, skip, size, extra);
 	*size /= world_input->token_size;
	return addr;
}

void
drake_input_discard(world, world, input)(size_t size)
{
	pelib_cfifo_discard(char)(world_input->queue, size * world_input->token_size);
}

int
drake_task_autokill(drake_task_tp task)
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
drake_task_autosleep(drake_task_tp task)
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
drake_task_autoexit(drake_task_tp task)
{
	int sleep = drake_task_autosleep(task);
	int kill = drake_task_autokill(task);
	return sleep | kill;
}

static void init_schedule()
{
	// Reinitialize schedule
	unsigned int i;
	drake_exec_task_t *schedule = schedule_start[drake_platform_core_id()];
	for(i = 0; i < number_of_task_instances[drake_platform_core_id()] - 1; i++)
	{
		schedule[i].next = &schedule[i + 1];
		schedule[i].round_next = &schedule[i + 1];
	}
	schedule[i].next = NULL;
	schedule[i].round_next = NULL;
}

int
drake_application_init(void* aux)
{
	debug("Init");
	int success;

	init_schedule();

	// This will come when the application is initialized
	// By now it should be possible to initialize all tasks
	unsigned int core = drake_platform_core_id();
	drake_exec_task_t *exec_task;
	for(exec_task = schedule_start[0]; exec_task != NULL; exec_task = exec_task->next) 
	{
		drake_task_t *task = exec_task->task;
		if(task->state == DRAKE_TASK_STATE_INIT)
		{
			if(task->init(aux) == 0)
			{
				debug("An error occurred while initializing task");
				//abort();
				success = 0;
			}
			task->state = DRAKE_TASK_STATE_START;
		}
	}
	debug("End Init");

	return success;
}

int
drake_application_run()
{
	debug("Run");
	unsigned int core = drake_platform_core_id();
	unsigned int i;
	unsigned int success = 1;
	drake_exec_task_t root;
	root.next = schedule_start[core];
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
						if(task->start())
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
						int state = task->run();
						if(state & (DRAKE_TASK_CONDITION_SLEEP | DRAKE_TASK_CONDITION_KILL))
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
						if(state & DRAKE_TASK_CONDITION_KILL)
						{
							// Marked task as killed of effectively killed
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
		debug("End of round");

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
					success = success && task->kill();
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
	debug("End Run");
	return success;
}

int
drake_application_destroy()
{
	debug("Destroy");
	int success;
	init_schedule();

	// This will come when the application is initialized
	// By now it should be possible to initialize all tasks
	unsigned int core = drake_platform_core_id();
	drake_exec_task_t *exec_task;
	for(exec_task = schedule_start[0]; exec_task != NULL; exec_task = exec_task->next) 
	{
		drake_task_t *task = exec_task->task;
		if(task->state < DRAKE_TASK_STATE_DESTROY)
		{
			if(task->destroy() == 0)
			{
				success = 0;
			}
			task->state = DRAKE_TASK_STATE_DESTROY;
		}
	}
	debug("End Destroy");
	return 1;
}
