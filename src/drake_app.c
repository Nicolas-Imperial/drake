#include <drake.h>
#include <drake/platform.h>

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

#define HELLO 0
#define WORLD 1

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

struct drake_application*
drake_application_build()
{
	drake_memory_t type = (drake_memory_t)(DRAKE_MEMORY_PRIVATE | DRAKE_MEMORY_SMALL_CHEAP);
	unsigned int level = 0;

	drake_application_t *app = drake_platform_get_application_details();
	app->number_of_cores = 1;
	app->core_private_link = (drake_core_private_link_t*)drake_platform_malloc(sizeof(drake_core_private_link_t) * app->number_of_cores, drake_platform_core_id(), type, level);
	app->core_private_link[0].number_of_links = 1;
	app->core_private_link[0].link = (drake_abstract_link_t*)drake_platform_malloc(sizeof(drake_abstract_link_t) * app->core_private_link[0].number_of_links, drake_platform_core_id(), type, level);

	app->number_of_tasks = 2;
	app->task = (drake_task_t*)drake_platform_malloc(sizeof(drake_task_t) * app->number_of_tasks, drake_platform_core_id(), type, level);
	app->task[HELLO].init = drake_init(hello, hello);
	app->task[HELLO].start = drake_start(hello, hello);
	app->task[HELLO].run = drake_run(hello, hello);
	app->task[HELLO].kill = drake_kill(hello, hello);
	app->task[HELLO].destroy = drake_destroy(hello, hello);
	app->task[HELLO].asleep = 0;
	app->task[HELLO].state = DRAKE_TASK_STATE_INIT;
	app->task[HELLO].name = "hello";
	app->task[HELLO].instance = NULL;
	app->task[HELLO].number_of_input_links = 0;
	app->task[HELLO].number_of_output_links = 1;
	app->task[HELLO].number_of_input_distributed_links = 0;
	app->task[HELLO].number_of_output_distributed_links = 0;
	
	app->task[WORLD].init = drake_init(world, world);
	app->task[WORLD].start = drake_start(world, world);
	app->task[WORLD].run = drake_run(world, world);
	app->task[WORLD].kill = drake_kill(world, world);
	app->task[WORLD].destroy = drake_destroy(world, world);
	app->task[WORLD].asleep = 0;
	app->task[WORLD].state = DRAKE_TASK_STATE_INIT;
	app->task[WORLD].name = "world";
	app->task[WORLD].instance = NULL;
	app->task[WORLD].number_of_input_links = 1;
	app->task[WORLD].number_of_output_links = 0;
	app->task[HELLO].number_of_input_distributed_links = 0;
	app->task[HELLO].number_of_output_distributed_links = 0;

	app->core_private_link[0].link[0].producer = &app->task[HELLO];
	app->core_private_link[0].link[0].consumer = &app->task[WORLD];
	app->core_private_link[0].link[0].queue = (cfifo_t(char)*)drake_platform_malloc(sizeof(cfifo_t(char)), drake_platform_core_id(), type, level);
	app->core_private_link[0].link[0].queue->capacity = 14;
	app->core_private_link[0].link[0].queue->buffer = (char*)drake_platform_malloc(app->core_private_link[0].link[0].queue->capacity * sizeof(char), drake_platform_core_id(), type, level);
	pelib_init(cfifo_t(char))(app->core_private_link[0].link[0].queue);
	app->core_private_link[0].link[0].token_size = 1;
	app->core_private_link[0].link[0].type = (drake_memory_t)(DRAKE_MEMORY_PRIVATE | DRAKE_MEMORY_SMALL_CHEAP);
	app->core_private_link[0].link[0].core = 0;
	app->core_private_link[0].link[0].level = 0;

	app->task[HELLO].input_link = NULL;
	app->task[HELLO].output_link = (drake_abstract_link_t**)drake_platform_malloc(sizeof(drake_abstract_link_t*) * app->task[HELLO].number_of_output_links, drake_platform_core_id(), type, level);
	app->task[HELLO].output_link[0] = &app->core_private_link[0].link[0];
	app->task[HELLO].input_distributed_link = NULL;
	app->task[HELLO].output_distributed_link = NULL;
	
	app->task[WORLD].input_link = (drake_abstract_link_t**)drake_platform_malloc(sizeof(drake_abstract_link_t*) * app->task[WORLD].number_of_input_links, drake_platform_core_id(), type, level);
	app->task[WORLD].input_link[0] = &app->core_private_link[0].link[0];
	app->task[WORLD].output_link = NULL;
	app->task[WORLD].input_distributed_link = NULL;
	app->task[WORLD].output_distributed_link = NULL;

	app->core_input_distributed_link = (drake_core_input_distributed_link_t*)drake_platform_malloc(sizeof(drake_core_input_distributed_link_t) * app->number_of_cores, drake_platform_core_id(), type, level);
	app->core_input_distributed_link[0].number_of_links = 0;
	app->core_input_distributed_link[0].link = NULL;
	app->core_output_distributed_link = (drake_core_output_distributed_link_t*)drake_platform_malloc(sizeof(drake_core_output_distributed_link_t) * app->number_of_cores, drake_platform_core_id(), type, level);
	app->core_output_distributed_link[0].number_of_links = 0;
	app->core_output_distributed_link[0].link = NULL;

	app->number_of_task_instances = (unsigned int*)drake_platform_malloc(sizeof(unsigned int) * app->number_of_cores, drake_platform_core_id(), type, level);
	app->number_of_task_instances[0] = 3;
	app->schedule = (drake_exec_task_t**)drake_platform_malloc(sizeof(drake_exec_task_t*) * app->number_of_cores, drake_platform_core_id(), type, level);
	app->schedule[0] = (drake_exec_task_t*)drake_platform_malloc(sizeof(drake_exec_task_t) * 3, drake_platform_core_id(), type, level);
	app->schedule[0][0].task = &app->task[HELLO];
	app->schedule[0][0].start_time = 0;
	app->schedule[0][0].width = 1;
	app->schedule[0][0].frequency = 1;
	app->schedule[0][0].next = &app->schedule[0][1];
	app->schedule[0][0].round_next = &app->schedule[0][1];
	app->schedule[0][1].task = &app->task[WORLD];
	app->schedule[0][1].start_time = 1;
	app->schedule[0][1].width = 1;
	app->schedule[0][1].frequency = 1;
	app->schedule[0][1].next = &app->schedule[0][2];
	app->schedule[0][1].round_next = &app->schedule[0][2];
	app->schedule[0][2].task = &app->task[WORLD];
	app->schedule[0][2].start_time = 0;
	app->schedule[0][2].width = 1;
	app->schedule[0][2].frequency = 1;
	app->schedule[0][2].next = NULL;
	app->schedule[0][2].round_next = NULL;

	return app;
}

drake_task_tp
drake_task(hello, hello)()
{
	return &drake_platform_get_application_details()->task[0];
}

drake_task_tp
drake_task(world, world)()
{
	return &drake_platform_get_application_details()->task[1];
}

unsigned int
drake_application_number_of_tasks()
{
	return drake_platform_get_application_details()->number_of_tasks;
}

size_t
drake_output_capacity(hello, hello, output)()
{
	return drake_buffer_capacity(drake_platform_get_application_details()->task[0].output_link[0]);
}

size_t
drake_output_available(hello, hello, output)()
{
	return drake_buffer_free(drake_platform_get_application_details()->task[0].output_link[0]);
}

size_t
drake_output_available_continuous(hello, hello, output)()
{
	return drake_buffer_free_continuous(drake_platform_get_application_details()->task[0].output_link[0]);
}

char*
drake_output_buffer(hello, hello, output)(size_t *size, char **extra)
{
	return (char*)drake_buffer_freeaddr(drake_platform_get_application_details()->task[0].output_link[0], size, (void**)extra);
}

void
drake_output_commit(hello, hello, output)(size_t size)
{
	drake_buffer_commit(drake_platform_get_application_details()->task[0].output_link[0], size);
}

size_t
drake_input_capacity(world, world, input)()
{
	return drake_buffer_capacity(drake_platform_get_application_details()->task[1].input_link[0]);
}

size_t
drake_input_available(world, world, input)()
{
	return drake_buffer_available(drake_platform_get_application_details()->task[1].input_link[0]);
}

size_t
drake_input_available_continuous(world, world, input)()
{
	return drake_buffer_available_continuous(drake_platform_get_application_details()->task[1].input_link[0]);
}

char*
drake_input_buffer(world, world, input)(size_t skip, size_t *size, char **extra)
{
	return (char*)drake_buffer_availableaddr(drake_platform_get_application_details()->task[1].input_link[0], skip, size, (void**)extra);
}

void
drake_input_discard(world, world, input)(size_t size)
{
	drake_buffer_discard(drake_platform_get_application_details()->task[1].input_link[0], size);
}

