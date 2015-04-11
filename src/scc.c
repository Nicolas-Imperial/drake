#include <stdio.h>
#include <stdarg.h>

#include <snekkja/platform.h>
#include <pelib_scc.h>
#include <scc_printf.h>
#include <RCCE.h>
#include <sync.h>
#include <scc.h>

static volatile void* mpb;
static void* alloc_ptr;

int snekkja_arch_init(void* obj)
{
	// Makes sure something has been transmitted
	if(obj == NULL)
	{
		pelib_scc_errprintf("Snekkja for SCC requires the commandline parameters to transmit to RCCE\n");
		//abort();
	}

	scc_args_t *args = (scc_args_t*) obj;
	pelib_scc_init((int*)args->argc, args->argv);

	// Initiate redirection
	//pelib_scc_init_redirect();
	//pelib_scc_set_redirect();

	return 0;
}

int snekkja_arch_finalize(void* obj)
{
	// Stop and finalize redirection
	pelib_scc_stop_redirect();
	pelib_scc_finalize_redirect();

	// Shutdown RCCE
	pelib_scc_finalize();

	return 0;
}

volatile void* snekkja_arch_alloc(size_t size, int core)
{
	// Not implemented
	pelib_scc_errprintf("Not implemented for SCC: %s(size_t size, int core)\n", __FUNCTION__);
	abort();
	return NULL;
}

int snekkja_arch_free(volatile void* addr)
{
	// Not implemented
	pelib_scc_errprintf("Not implemented for SCC: %s(void* addr)\n", __FUNCTION__);
	abort();
	return 0;
}

int snekkja_arch_pull(volatile void* addr)
{
	(void*)(addr);
	pelib_scc_cache_invalidate();

	// This cannot go wrong
	return 1;
}

int snekkja_arch_commit(volatile void* addr)
{
	(void*)(addr);
	// Wherever you wrote, just invalidate the line
	pelib_scc_force_wcb();

	// This cannot go wrong
	return 1;
}

int
snekkja_core()
{
	return RCCE_ue();
}

void
snekkja_stdout(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(pelib_scc_get_stdout(), format, args);
	va_end(args);
}

void
snekkja_stderr(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(pelib_scc_get_stderr(), format, args);
	va_end(args);
}

void
snekkja_barrier(void* channel)
{
	RCCE_barrier(&RCCE_COMM_WORLD);
}

void
snekkja_exclusive_begin()
{
	PELIB_SCC_CRITICAL_BEGIN
}

void
snekkja_exclusive_end()
{
	PELIB_SCC_CRITICAL_END
}
