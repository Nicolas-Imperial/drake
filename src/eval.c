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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#include <drake/stream.h>
#include <stddef.h>
//#include <drake/platform.h>
//#include <drake/link.h>
#include <drake.h>
#include <drake/eval.h>
//#include <pelib/integer.h>

// APPLICATION is aleady defined by a compile switch
//#define APPLICATION merge
// Cannot define this marker through the APPLICATION symbol
// This means that drake.h should not be included a second time
//#define DONE_merge 1

#if 0 
#define debug(var) printf("[%s:%s:%d:CORE %zu] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:CORE %zu] %s = \"%X\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:CORE %zu] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:CORE %zu] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_core(), #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

typedef struct
{
	args_t platform, application;
	char **time_output_file, **power_output_file;
	size_t *core_ids;
} arguments_t;

static arguments_t
parse_arguments(int argc, char** argv)
{
	arguments_t args;
	size_t i;

	// default values
	args.time_output_file = malloc(sizeof(char*) * drake_core_max());
	args.power_output_file = malloc(sizeof(char*) * drake_core_max());
	for(i = 0; i < drake_core_max(); i++)
	{
		args.time_output_file[i] = NULL;
		args.power_output_file[i] = NULL;
	}

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

		if(strcmp(argv[0], "--core-output") == 0)
		{
			// Proceed to next argument
			argv++;
			size_t core;
			core = atoi(argv[0]);

			if(strcmp((argv + 1)[0], "--time") == 0)
			{
				argv += 2;
				args.time_output_file[core] = argv[0];
			}

			if(strcmp((argv + 1)[0], "--power") == 0)
			{
				argv += 2;
				args.power_output_file[core] = argv[0];
			}

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
	drake_stream_t stream;
	drake_stream_create(&stream, PIPELINE);

	// Allocate monitoring buffers
	init = malloc(sizeof(drake_time_t) * drake_task_number(PIPELINE)());
	start = malloc(sizeof(drake_time_t) * drake_task_number(PIPELINE)());
	run = malloc(sizeof(drake_time_t) * drake_task_number(PIPELINE)());
	killed = malloc(sizeof(drake_time_t) * drake_task_number(PIPELINE)());
	destroy = malloc(sizeof(drake_time_t) * drake_task_number(PIPELINE)());
	execute = malloc(sizeof(int) * drake_task_number(PIPELINE)());
	for(i = 0; i < drake_task_number(PIPELINE)(); i++)
	{
		init[i] = drake_time_alloc();
		start[i] = drake_time_alloc();
		run[i] = drake_time_alloc();
		killed[i] = drake_time_alloc();
		destroy[i] = drake_time_alloc();
		execute[i] = 0;
	}

	// Initialize stream (The scc requires this phase to not be run in parallel because of extensive IOs when loading input)
	drake_stream_init(&stream, &args.application);

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
	if(args.time_output_file[drake_core()] != NULL)
	{
		out = fopen(args.time_output_file[drake_core()], "w");
	}
	else
	{
		out = stdout;
	}
	fprintf(out, "core__task__init__start__run__kill__destroy__global [*,*]\n:\t0\t1\t2\t3\t4\t5\t6\t7\t:=\n");
	for(i = 0; i < drake_task_number(PIPELINE)(); i++)
	{
		if(execute[i] != 0)
		{
			fprintf(out, "%zu %zu %s ", i + 1, drake_core(), drake_task_name(PIPELINE)(i + 1));
			drake_time_printf(out, init[i]);
			fprintf(out, " ");
			drake_time_printf(out, start[i]);
			fprintf(out, " ");
			drake_time_printf(out, run[i]);
			fprintf(out, " ");
			drake_time_printf(out, killed[i]);
			fprintf(out, " ");
			drake_time_printf(out, destroy[i]);
			fprintf(out, " ");
			drake_time_printf(out, global);
			fprintf(out, "\n");
		}
	}
	fprintf(out, ";\n");
	if(args.time_output_file[drake_core()] != NULL)
	{
		fclose(out);
	}

	// Output power data
	if(args.power_output_file[drake_core()] != NULL)
	{
		out = fopen(args.power_output_file[drake_core()], "w");
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
	if(args.power_output_file[drake_core()] != NULL)
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

