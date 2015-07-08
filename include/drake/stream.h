#include <stddef.h>
#include <drake/mapping.h>
#include <drake/processor.h>
#include <drake/platform.h>

#ifndef DRAKE_STREAM_H
#define DRAKE_STREAM_H

typedef struct {
	void* base_ptr;
	size_t size;
	size_t* stack_ptr;
} drake_stack_t;

typedef struct {
	mapping_t *mapping;
	processor_t *proc;
	drake_stack_t *stack;
	size_t local_memory_size;
	drake_time_t stage_time, stage_start_time, stage_stop_time, stage_sleep_time;
} drake_stream_t;

drake_stream_t drake_stream_create(void*);
int drake_stream_init(drake_stream_t*, void*);
int drake_stream_run(drake_stream_t*);
int drake_stream_destroy(drake_stream_t*, void*);

#endif
