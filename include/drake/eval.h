#include <drake/platform.h>

#ifndef DRAKE_EVAL
#define DRAKE_EVAL

typedef struct args
{
	size_t argc;
	char **argv;
} args_t;

// pthread *hates* when a pointer is named "kill".
drake_time_t *init, *start, *run, *killed, *destroy;

int *execute;

#endif
