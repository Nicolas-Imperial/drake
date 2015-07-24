#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <drake/stream.h>
#include <stddef.h>
#include <drake/platform.h>
#include <drake/link.h>
#include <drake/eval.h>
#include <pelib/integer.h>

//#define APPLICATION merge
#include <drake.h>
//#define DONE_merge 1

#if 1
#define debug(var) printf("[%s:%s:%d:CORE %d] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, 00, #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:CORE %d] %s = \"%X\"\n", __FILE__, __FUNCTION__, __LINE__, 00, #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:CORE %d] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, 00, #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:CORE %d] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, 00, #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

#if 1
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
	drake_arch_init(&args1);
	drake_stream_t stream = drake_stream_create(APPLICATION);

	// Allocate monitoring buffers
	size_t i;
	init = malloc(sizeof(drake_time_t) * drake_task_number());
	start = malloc(sizeof(drake_time_t) * drake_task_number());
	work = malloc(sizeof(drake_time_t) * drake_task_number());
	killed = malloc(sizeof(drake_time_t) * drake_task_number());
	run = malloc(sizeof(int) * drake_task_number());
	for(i = 0; i < drake_task_number(); i++)
	{
		init[i] = drake_time_alloc();
		start[i] = drake_time_alloc();
		work[i] = drake_time_alloc();
		killed[i] = drake_time_alloc();
		run[i] = 0;
	}

	// Initialize stream (The scc requires this phase to not be run in parallel because of extensive IOs when loading input)
	drake_exclusive_begin();
	drake_stream_init(&stream, argv[1]);
	drake_exclusive_end();

	// Measure global time
	drake_time_t global_begin = drake_time_alloc();
	drake_time_t global_end = drake_time_alloc();
/*
*/

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
	size_t collected;
	collected = drake_platform_power_end(power);

	// Compute and output statistics
	// Time
	drake_time_t global = drake_time_alloc();
	drake_time_substract(global, global_end, global_begin);
	
	drake_exclusive_begin();
	if(collected > SAMPLES)
	{
		drake_stderr("[ERROR] Insufficient number of samples (%d) to store all power data (%d).\n", SAMPLES, collected);
	}

	drake_stdout("\"core\", \"task_name\", \"task_init\", \"task_start\", \"task_work\", \"task_killed\", \"time_global\", \"power_time\", \"power_core\", \"power_memory_controller\"\n");
	for(i = 0; i < drake_task_number(); i++)
	{
		if(run[i] != 0)
		{
			size_t j;
			for(j = 0; j < (collected < SAMPLES ? collected : SAMPLES); j++)
			{
				drake_stdout("%d, \"%s\", ", drake_core(), drake_task_name(i));
				drake_time_display(stdout, init[i]);
				drake_stdout(", ");
				drake_time_display(stdout, start[i]);
				drake_stdout(", ");
				drake_time_display(stdout, work[i]);
				drake_stdout(", ");
				drake_time_display(stdout, killed[i]);
				drake_stdout(", ");
				drake_time_display(stdout, global);
				drake_stdout(", ");
				drake_platform_power_display_line(stdout, power, j, ", ");
				drake_stdout("\n");
			}
		}
	}
	drake_exclusive_end();

	// cleanup
	drake_platform_power_destroy(power);
	drake_time_destroy(global);
	drake_time_destroy(global_begin);
	drake_time_destroy(global_end);

#if 0
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
#endif

	drake_stream_destroy(&stream);
	drake_arch_finalize(NULL);

	return EXIT_SUCCESS;
}
#else
void* dummy(void *args)
{
	for(;;)
	{
	}
	pthread_exit(NULL);
}

/*
void*
measure_power(void* arg)
{
	for(;;);
	return NULL;
	drake_power_t args = (drake_power_t)arg;
	const struct timespec period = {0, 100 * 1e6}; // 100 ms

	// Wait for kickstarter
	args->running = 0;
	int a = sem_wait(&args->run);
	debug_int(a);

#ifdef COPPERRIDGE
	InitAPI(0);  
	writeFpgaGrb(DVFS_CONFIG, DVFS_CONFIG_SETTING);
#endif

	double current_c, current_mc, voltage_c, voltage_mc;
	const struct timespec short_sleeptime = {0, 5 * 1e6}; // 5 ms
	double time_ms;

	struct timespec time;
	clock_gettime(CLOCK_MONOTONIC,&time);
	double start_ms = time.tv_sec * 1e3 + time.tv_nsec * 1e-6;

	args->running = 1;
	while(sem_trywait(&args->run) != 0)
	{
		if(args->collected < args->max_samples)
		{
			if(args->measurement & DRAKE_POWER_CORE != (int)0)
			{
#ifdef COPPERRIDGE
				voltage_c = readStatus(DVFS_STATUS_U3V3SCC);
				current_c = readStatus(DVFS_STATUS_I3V3SCC);
				args->power_core[args->collected] = voltage_c * current_c;
#else
				args->power_core[args->collected] = 0;
#endif
			}
			else
			{
				args->power_core[args->collected] = 0;
			}

			if(args->measurement & DRAKE_POWER_MEMORY_CONTROLLER != (int)0)
			{
#ifdef COPPERRIDGE
				voltage_mc = readStatus(DVFS_STATUS_U1V5);
				current_mc = readStatus(DVFS_STATUS_I1V5);
				args->power_mc[args->collected] = voltage_mc * current_mc;
#else
				args->power_mc[args->collected] = 0;
#endif
			}
			else
			{
				args->power_mc[args->collected] = 0;
			}
      
			// There are some unreasonably high spikes every 500-700 ms or so. We ignore these and wait for the value to settle.
			if(current_c * voltage_c > REASONABLE_C || current_mc * voltage_mc > REASONABLE_MC)
			{
				nanosleep(&short_sleeptime, NULL);
				continue;
			}

			clock_gettime(CLOCK_MONOTONIC,&time);
			time_ms = time.tv_sec * 1e3 +time.tv_nsec * 1e-6 - start_ms;
			args->time[args->collected] = time_ms;
		}

		// Sleep until next measurement
		nanosleep(&period, NULL);

		// Count the number of iterations
		args->collected++;
	}

	args->running = 0;
#ifdef COPPERRIDGE
	writeFpgaGrb(DVFS_CONFIG, DVFS_CONFIG_RESET); // Go back to no measurement
#endif

	return NULL;
}
*/

//drake_power_t
//drake_platform_power_init(size_t samples, int measurement)
int
main(int argc, char** argv)
{
	//drake_power_t tracker = malloc(sizeof(struct drake_power));
	/*
	tracker->power_core = malloc(sizeof(double) * samples);
	tracker->power_mc = malloc(sizeof(double) * samples);
	tracker->time = malloc(sizeof(double) * samples);
	tracker->max_samples = samples;
	tracker->collected = 0;
	tracker->measurement = measurement;
	sem_init(&tracker->run, 0, 0);
	*/

	pthread_t t;

	//int retval = pthread_create(&tracker->thread, &attr, dummy, NULL);
	int retval = pthread_create(&t, NULL, dummy, NULL);
  	pthread_join(t, NULL);
	/*
	if(retval != 0)
	{
		free(tracker->power_core);
		free(tracker->power_mc);
		free(tracker->time);
		free(tracker);
		tracker = NULL;
	}

	*/
	return NULL;
}
#endif
