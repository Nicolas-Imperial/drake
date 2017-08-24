#include <drake.h>

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

// QUESTION: Should we have it statically declared or allocated in fast private memory?
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
static unsigned int number_of_task_instances[] = { 3 };

static drake_application_t app;

struct drake_application*
drake_application_get()
{
	app.number_of_cores = number_of_cores;
	app.core_shared_queue = core_shared_queue;
	app.core_private_link = core_private_link;
	app.core_input_link = core_input_link;
	app.core_output_link = core_output_link;
	app.number_of_task_instances = number_of_task_instances;
	app.schedule_start = schedule_start;

	return &app;
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

unsigned int
drake_application_number_of_tasks()
{
	return number_of_tasks;
}

size_t
drake_output_capacity(hello, hello, output)()
{
	return drake_buffer_capacity(hello_output);
}

size_t
drake_output_available(hello, hello, output)()
{
	return drake_buffer_free(hello_output);
}

size_t
drake_output_available_continuous(hello, hello, output)()
{
	return drake_buffer_free_continuous(hello_output);
}

char*
drake_output_buffer(hello, hello, output)(size_t *size, char **extra)
{
	return (char*)drake_buffer_freeaddr(hello_output, size, (void**)extra);
}

void
drake_output_commit(hello, hello, output)(size_t size)
{
	drake_buffer_commit(hello_output, size);
}

size_t
drake_input_capacity(world, world, input)()
{
	return drake_buffer_capacity(world_input);
}

size_t
drake_input_available(world, world, input)()
{
	return drake_buffer_available(world_input);
}

size_t
drake_input_available_continuous(world, world, input)()
{
	return drake_buffer_available_continuous(world_input);
}

char*
drake_input_buffer(world, world, input)(size_t skip, size_t *size, char **extra)
{
	return (char*)drake_buffer_availableaddr(world_input, skip, size, (void**)extra);
}

void
drake_input_discard(world, world, input)(size_t size)
{
	drake_buffer_discard(world_input, size);
}

