#include <stdlib.h>

#include <drake/stream.h>
#include <stddef.h>
#include <merge/merge.h>
#include <drake/platform.h>
#include <drake/link.h>
#include <drake/eval.h>
#include <pelib/integer.h>

#define APPLICATION merge
#include <drake/schedule.h>
#define DONE_merge 1

typedef struct args
{
	size_t *argc;
	char ***argv;
} args_t;

int
main(size_t argc, char **argv)
{
	args_t args1;
	args1.argc = &argc;
	args1.argv = &argv;
	drake_stream_t stream = drake_stream_create(APPLICATION, (void*)&args1);

	merge_t args2;
	args2.filename = argv[2];
	drake_exclusive_begin();
	drake_stream_init(&stream, &args2);
	drake_exclusive_end();

	// Measure global time
	drake_time_t global_begin = drake_time_alloc();
	drake_time_t global_end = drake_time_alloc();

	// Measure power consumption
	drake_power_t power = drake_platform_power_init(SAMPLES, DRAKE_POWER_CORE | DRAKE_POWER_MEMORY_CONTROLLER);

	// Begin power measurement
	drake_platform_power_begin(power);

	// Begin time measurement
	drake_get_time(global_begin);

	// Run the pipeline
	drake_stream_run(&stream);

	// Stop time measurement
	drake_get_time(global_end);

	// Stop energy measurement
	size_t collected = drake_platform_power_end(power);

	// Compute and output statistics
	// Time
	drake_time_t global = drake_time_alloc();
	drake_time_substract(global, global_end, global_begin);
	
	if(collected > SAMPLES)
	{
		drake_stderr("[ERROR] Insufficient number of samples (%d) to store all power data (%d).\n", SAMPLES, collected);
	}

	drake_stdout("\"core\", \"time_global\", \"power_time\", \"power_core\", \"power_memory_controller\"\n");
	size_t i;
	for(size_t i = 0; i < (collected < SAMPLES ? collected : SAMPLES); i++)
	{
		drake_stdout("%d, ", drake_core());
		drake_time_display(stdout, global);
		drake_stdout(", ");
		drake_platform_power_display_line(stdout, power, i, ", ");
		drake_stdout("\n");
	}

	// cleanup
	drake_platform_power_destroy(power);
	drake_time_destroy(global);
	drake_time_destroy(global_begin);
	drake_time_destroy(global_end);

/*
			task->start_presort,
			task->stop_presort,
			task->start_time,
			task->stop_time,
			task->step_init,
			task->step_start,
			task->step_check,
			task->step_work,
			task->step_push,
			task->step_killed,
			task->step_zombie,
			task->step_transition
*/

	drake_stream_destroy(&stream, NULL);

	return EXIT_SUCCESS;
}

