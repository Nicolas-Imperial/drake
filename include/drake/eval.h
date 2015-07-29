#ifndef DRAKE_EVAL
#define DRAKE_EVAL

typedef struct args
{
	size_t argc;
	char **argv;
} args_t;

// pthread in icc 8.1/gcc 3.4.0 *hate* when a pointer is named "kill"
// Forbidden: kill
drake_time_t *init, *start, *work, *killed;
int *run;

#endif
