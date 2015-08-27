#include <drake/platform.h>

#ifndef DRAKE_EVAL
#define DRAKE_EVAL

typedef struct args
{
	size_t argc;
	char **argv;
} args_t;

drake_time_t *init, *start, *run, *kill, *destroy;
int *execute;

#endif
