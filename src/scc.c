#include <stdio.h>
#include <stdarg.h>

#include <architecture.h>
#include <pelib_scc.h>
#include <scc_printf.h>
#include <RCCE.h>

static volatile void* mpb;
static void* alloc_ptr;

struct args
{
	int *argc;
	char ***argv;
};
typedef struct args args_t;

int snekkja_arch_init(void* obj)
{
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	// Makes sure something has been transmitted
	if(obj == NULL)
	{
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
		pelib_scc_errprintf("Snekkja for SCC requires the commandline parameters to transmit to RCCE\n");
		//abort();
	}

	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	args_t *args = (args_t*) obj;
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	pelib_scc_init(args->argc, args->argv);
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);

	// Initiate redirection
	pelib_scc_init_redirect();
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
	pelib_scc_set_redirect();
	snekkja_stderr("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);

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
