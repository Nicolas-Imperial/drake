#include <snekkja/stream.h>
#include <pelib_scc.h>
#include <sync.h>
#include <stddef.h>
#include <scc.h>
#include <merge.h>
#include <snekkja/platform.h>
#include <snekkja/link.h>
#include <integer.h>

#if DEBUG
//int printf_enabled = 0;
#define assert_equal(value, expected, abort_on_failure) if(value != expected) { snekkja_stderr("[CORE %d][%s:%s:%d] Expected %s == %d, got %s == %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #expected, expected, #value, value); if (abort_on_failure) { abort(); } }
#define assert_different(value, expected, abort_on_failure) if(value == expected) { snekkja_stderr("[CORE %d][%s:%s:%d] Got %s == %d, expected different than %s == %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #value, value, #expected, expected); if (abort_on_failure) { abort(); } }
#define assert_geq(value, reference, abort_on_failure) if(value < reference) { snekkja_stderr("[CORE %d][%s:%s:%d] Got %s == %d, strictly lower than than %s == %d, but expected greater or equal (>=\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #value, value, #reference, reference); if (abort_on_failure) { abort(); } }
#define debug snekkja_stdout("[CORE %d][%s:%s:%d] %d out of %d tasks left\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, active_tasks, proc->handled_nodes);
#define debug_task snekkja_stdout("[CORE %d][%s:%s:%d] Task %d state %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, task->id, task->status);
#define debug_task_output snekkja_stdout("[CORE %d][%s:%s:%d] Task %d to task %d\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, task->id, link->link->task->id);
#define printf_addr(addr) snekkja_stdout("[CORE %d][%s:%s:%d] %s = %X\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #addr, addr)
#define printf_int(integer) snekkja_stdout("[CORE %d][%s:%s:%d] %s = %d (signed), %u (unsigned)\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #integer, integer, integer)
#define printf_str(str) snekkja_stdout("[CORE %d][%s:%s:%d] %s = %s\n", snekkja_core(), __FILE__, __FUNCTION__, __LINE__, #str, str);
#define PRINTF_FEEDBACK 0
#define PRINTF_PUSH 0
#define PRINTF_CHECK_IN 0
#define PRINTF_CHECK_OUT 0
#define CHECK_CORRECT              1
#else
//int printf_enabled = 0;
#define assert_equal(value, expected, abort_on_failure)
#define assert_different(value, expected, abort_on_failure)
#define assert_geq(value, reference, abort_on_failure)
#define debug
#define predebug
#define debug_task
#define debug_task_output
#define printf_addr(addr)
#define printf_int(integer)
#define printf_str(str)
#endif

#define CHECK_CORRECT              1

static int
int_cmp(const void *a, const void *b)
{
	return *(int*)a - *(int*)b;
}

int
main(size_t argc, char **argv)
{
	scc_args_t args1;
	args1.argc = &argc;
	args1.argv = &argv;
	snekkja_stream_t stream = snekkja_stream_create((void*)&args1);

	merge_t args2;
	args2.filename = argv[2];
	snekkja_exclusive_begin();
	snekkja_stream_init(&stream, &args2);
	snekkja_exclusive_end();

	// Not actually necessary, because of the exclusive section above, that ends with a barrier
	snekkja_barrier(NULL);

	snekkja_stream_run(&stream);

	size_t i, j, k, memory_consistency_errors;
	unsigned long long int start, stop;
	processor_t *proc = stream.proc;
	task_t *task;
	link_tp link;
	array_t(int) *array;

#if CHECK_CORRECT
	int final_size;
	array_t(int) *sorted, *ref;
	int error_detected = 0;
	int got = 0;

#if !SORT_SEQUENTIAL
	for(i = 0; i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		for(j = 0; j < pelib_array_length(link_tp)(task->succ); j++)
		{
			link = pelib_array_read(link_tp)(task->succ, j);

			if(link->cons == NULL)
			{
				array = pelib_array_from_cfifo(int)(link->buffer);
			}
		}
	}
#endif

	PELIB_SCC_CRITICAL_BEGIN
	if(array != NULL)
	{
		// Load input and qsort it using RCCE_qsort
		ref = pelib_array_preloadfilenamebinary(int)(argv[2]);
		final_size = pelib_array_length(int)(ref);
		pelib_free_struct(array_t(int))(ref);
		final_size = final_size / 8;

		if(final_size != pelib_array_length(int)(array))
		{
			snekkja_stderr("[CORE %d][ERROR] The length of sorted array (%d) is different than expected length (%d)\n", snekkja_core(), pelib_cfifo_length(int)(*link->buffer), final_size);
			abort();
			error_detected = 1;
		}
		//ref = pelib_array_loadfilenamewindowbinary(int)(argv[2], final_size * octant_id(snekkja_core()), final_size); // Transformed
		ref = pelib_array_loadfilenamewindowbinary(int)(argv[2], 0, final_size);

		// qsort so we can compare our output to a correct output
		qsort((char*)ref->data, final_size, sizeof(int), int_cmp);

		for(k = 0; k < final_size; k++)
		{
			if(ref->data[k] != array->data[k])
			{
				//abort();
				error_detected = 1;
				got = array->data[k];
				break;
			}
		}
#if EXPORT_REFERENCE
		outputfile = malloc(strlen(argv[2]) + 4 + 3);
		sprintf(outputfile, "%s.ref.%d", argv[2], octant_id(snekkja_core()));
		pelib_array_storefilename(int)(ref, outputfile);
		free(outputfile);
#endif
	}

#if EXPORT_OUTPUT
	if(array != NULL)
	{
		umask(022);
		// Save output file to disk
		outputfile = malloc(strlen(argv[2]) + 3);
		sprintf(outputfile, "%s.%d", argv[2], octant_id(snekkja_core()));
		pelib_array_storefilename(int)(array, outputfile);
		free(outputfile);
	}
#endif

	PELIB_SCC_CRITICAL_END
	snekkja_barrier(NULL);

	if(!error_detected)
	{
		snekkja_stderr("[CORE %d] Everything went OK\n", snekkja_core());
	}
	else
	{
		snekkja_stderr("[CORE %d][ERROR] The sorted array did not match reference at index %d (got %d, expected %d)\n", snekkja_core(), k, got, ref->data[k]);
	}
#endif
	snekkja_stderr("[CORE %d] Now finishing...\n", snekkja_core());

#if MEASURE
#if !SORT_SEQUENTIAL
	snekkja_stdout("%% table_columns = \
		core \
		proc_id \
		octant_id \
		errors \
		task_id \
		start \
		stop \
		presort_start \
		presort_stop \
		task_start \
		task_stop \
		task_step_init \
		task_step_start \
		task_step_check \
		task_step_work \
		task_step_push \
		task_step_killed \
		task_step_zombie \
		task_step_transition \
	\n");
	for(i = 0; i < proc->handled_nodes; i++)
	{
		task = proc->task[i];
		snekkja_stdout(
			"%d %d %d %d %d %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu %llu\n",
			snekkja_core(),
			proc->id,
			//octant_id(snekkja_core()), // Transformed
			0,
			memory_consistency_errors,
			task->id,
			start,
			stop,
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
		);
	}
#else
	if(core_id_in_octant(snekkja_core()) == 0)
	{
		snekkja_stdout("%% table_columns = \
			core \
			proc_id \
			octant_id \
			start \
			stop \
		\n");
		snekkja_stdout(
			"%d %d %d %llu %llu\n",
			snekkja_core(),
			0,
			octant_id(snekkja_core()),
			start,
			stop
		);
	}
#endif
#endif
	
	//pelib_scc_stop_redirect();
	//pelib_scc_finalize_redirect();


	snekkja_stream_destroy(&stream, NULL);
	return error_detected;
	return 0;
}

