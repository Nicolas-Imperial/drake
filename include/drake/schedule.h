#include <drake/task.h>

#ifndef DRAKE_SCHEDULE_H
#define DRAKE_SCHEDULE_H

typedef struct {
	size_t id;
	double start_time;
	double frequency;
} drake_schedule_task_t;
//typedef struct drake_schedule_task drake_schedule_task_t;

extern size_t _drake_p;
extern size_t _drake_n;
extern double _drake_stage_time;
extern size_t *_drake_tasks_in_core;
extern size_t *_drake_consumers_in_core;
extern size_t *_drake_producers_in_core;
extern size_t *_drake_consumers_in_task;
extern size_t *_drake_producers_in_task;
extern size_t **_drake_consumers_id;
extern size_t **_drake_producers_id;
extern size_t *_drake_remote_consumers_in_task;
extern size_t *_drake_remote_producers_in_task;
extern drake_schedule_task_t **_drake_schedule;
extern int *_drake_task_frequency;

void* drake_function(size_t id, task_status_t state);

void drake_schedule_init();
void drake_schedule_destroy();

#endif
