#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <drake/stream.h>
#include <stddef.h>
#include <drake/platform.h>
#include <drake/link.h>
#include <drake/eval.h>
#include <pelib/integer.h>

// APPLICATION is aleady defined by a compile switch
//#define APPLICATION merge
#include <drake.h>
// Cannot define this marker through the APPLICATION symbol
// This means that drake.h should not be included a second time
//#define DONE_merge 1

#if 1 
#define debug(var) printf("[%s:%s:%d:CORE %d] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:CORE %d] %s = \"%X\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:CORE %d] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:CORE %d] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

typedef struct
{
	args_t platform, application;
	char *time_output_file, *power_output_file;
} arguments_t;

static arguments_t
parse_arguments(int argc, char** argv)
{
	arguments_t args;

	// default values
	args.time_output_file = NULL;
	args.power_output_file = NULL;

	args.application.argc = 0;
	args.application.argv = malloc(sizeof(char*) * 1);
	args.application.argv[0] = NULL;

	args.platform.argc = 0;
	args.platform.argv = malloc(sizeof(char*) * 1);
	args.platform.argv[0] = NULL;

	for(argv++; argv[0] != NULL; argv++)
	{
		if(strcmp(argv[0], "--platform-args") == 0)
		{
			// Proceed to next argument
			argv++;

			while(argv[0] != NULL && strcmp(argv[0], "--") != 0)
			{
				// Backup the current argument stack and allocate one more argument slot
				char** tmp = args.platform.argv;
				args.platform.argv = malloc(sizeof(char*) * (args.platform.argc + 2));

				// Copy argument pointers as they are so far and free backup
				memcpy(args.platform.argv, tmp, sizeof(char*) * args.platform.argc);
				free(tmp);

				// Add the new argument and switch to next
				args.platform.argv[args.platform.argc] = *argv;
				argv++;

				// Increment platform argument counter and add the last NULL argument
				args.platform.argc++;
				args.platform.argv[args.platform.argc] = NULL;
			}

			continue;
		}
		
		if(strcmp(argv[0], "--application-args") == 0)
		{
			// Proceed to next argument
			argv++;

			while(argv[0] != NULL && strcmp(argv[0], "--") != 0)
			{
				// Backup the current argument stack and allocate one more argument slot
				char** tmp = args.application.argv;
				args.application.argv = malloc(sizeof(char*) * (args.application.argc + 2));

				// Copy argument pointers as they are so far and free backup
				memcpy(args.application.argv, tmp, sizeof(char*) * args.application.argc);
				free(tmp);

				// Add the new argument and switch to next
				args.application.argv[args.application.argc] = *argv;
				argv++;

				// Increment application argument counter and add the last NULL argument
				args.application.argc++;
				args.application.argv[args.application.argc] = NULL;
			}

			continue;
		}

		if(strcmp(argv[0], "--time-output") == 0)
		{
			// Proceed to next argument
			argv++;
			args.time_output_file = argv[0];
			continue;
		}

		if(strcmp(argv[0], "--power-output") == 0)
		{
			// Proceed to next argument
			argv++;
			args.power_output_file = argv[0];
			continue;
		}
	}

	return args;
}

int
main(size_t argc, char **argv)
{
	size_t i;
	arguments_t args = parse_arguments(argc, argv);

	drake_arch_init(&args.platform);
	drake_stream_t stream = drake_stream_create(APPLICATION);

	// Allocate monitoring buffers
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
	//drake_exclusive_begin();
	drake_stream_init(&stream, &args.application);
	//drake_exclusive_end();

	// Measure global time
	drake_time_t global_begin = drake_time_alloc();
	drake_time_t global_end = drake_time_alloc();

	// Measure power consumption
	drake_power_t power = drake_platform_power_init(SAMPLES, (1 << DRAKE_POWER_CHIP) | (1 << DRAKE_POWER_MEMORY_CONTROLLER));

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
	
	//drake_exclusive_begin();
	if(collected > SAMPLES)
	{
		fprintf(stderr, "[ERROR] Insufficient sample memory (%zu) to store all power data (%zu).\n", (size_t)SAMPLES, collected);
	}

	// Output time data
	FILE* out;
	if(args.time_output_file != NULL)
	{
		out = fopen(args.time_output_file, "w");
	}
	else
	{
		out = stdout;
	}
	for(i = 0; i < drake_task_number(); i++)
	{
		if(run[i] != 0)
		{
			fprintf(out, "%zu \"%s\" ", drake_core(), drake_task_name(i + 1));
			drake_time_printf(out, init[i]);
			fprintf(out, " ");
			drake_time_printf(out, start[i]);
			fprintf(out, " ");
			drake_time_printf(out, work[i]);
			fprintf(out, " ");
			drake_time_printf(out, killed[i]);
			fprintf(out, " ");
			drake_time_printf(out, global);
			fprintf(out, "\n");
		}
	}
	if(args.time_output_file != NULL)
	{
		fclose(out);
	}

	// Output power data
	if(args.power_output_file != NULL)
	{
		out = fopen(args.power_output_file, "w");
	}
	else
	{
		out = stdout;
	}

	// Chip and memory controller combined
	if((collected < SAMPLES ? collected : SAMPLES) > 0)
	{
		fprintf(out, "time__power [*,*]\n:\t0\t1\t:=\n");
		for(i = 0; i < (collected < SAMPLES ? collected : SAMPLES); i++)
		{
			fprintf(out, "%zu\t", i);
			drake_platform_power_printf_line_cumulate(out, power, i, (1 << DRAKE_POWER_CHIP) | (1 << DRAKE_POWER_MEMORY_CONTROLLER), "\t");
			fprintf(out, "\n");
		}
		fprintf(out, ";\n");

		// Chip only
		fprintf(out, "time__chippower [*,*]\n:\t0\t1\t:=\n");
		for(i = 0; i < (collected < SAMPLES ? collected : SAMPLES); i++)
		{
			fprintf(out, "%zu\t", i);
			drake_platform_power_printf_line_cumulate(out, power, i, (1 << DRAKE_POWER_CHIP), "\t");
			fprintf(out, "\n");
		}
		fprintf(out, ";\n");

		// Memory controller only
		fprintf(out, "time__mmcpower [*,*]\n:\t0\t1\t:=\n");
		for(i = 0; i < (collected < SAMPLES ? collected : SAMPLES); i++)
		{
			fprintf(out, "%zu\t", i);
			drake_platform_power_printf_line_cumulate(out, power, i, (1 << DRAKE_POWER_MEMORY_CONTROLLER), "\t");
			fprintf(out, "\n");
		}
		fprintf(out, ";\n");
	}
	if(args.power_output_file != NULL)
	{
		fclose(out);
	}
	//drake_exclusive_end();

	// cleanup
	drake_platform_power_destroy(power);
	drake_time_destroy(global);
	drake_time_destroy(global_begin);
	drake_time_destroy(global_end);

	drake_stream_destroy(&stream);
	drake_arch_finalize(NULL);

	return EXIT_SUCCESS;
}

