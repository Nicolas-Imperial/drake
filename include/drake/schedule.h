#include <drake/task.h>


#ifdef APPLICATION
#if PELIB_CONCAT_2(DONE_, APPLICATION) == 0

#define drake_function PELIB_##CONCAT_2(drake_function_, APPLICATION)
#define drake_schedule_init PELIB_##CONCAT_2(drake_schedule_init_, APPLICATION)
#define drake_schedule_destroy PELIB_##CONCAT_2(drake_schedule_destroy_, APPLICATION)
#define drake_task_number PELIB_##CONCAT_2(drake_task_number_, APPLICATION)
#define drake_task_name PELIB_##CONCAT_2(drake_task_name_, APPLICATION)
void* drake_function(size_t id, task_status_t state);
void drake_schedule_init();
void drake_schedule_destroy();
int drake_task_number();
char* drake_task_name(size_t);

#endif
#else
#endif

#ifndef DRAKE_SCHEDULE_H
#define DRAKE_SCHEDULE_H

typedef struct {
	size_t id;
	double start_time;
	double frequency;
} drake_schedule_task_t;
//typedef struct drake_schedule_task drake_schedule_task_t;

typedef struct {
	size_t core_number;
	size_t task_number;
	double stage_time;
	size_t *tasks_in_core;
	size_t *consumers_in_core;
	size_t *producers_in_core;
	size_t *consumers_in_task;
	size_t *producers_in_task;
	size_t **consumers_id;
	size_t **producers_id;
	size_t *remote_consumers_in_task;
	size_t *remote_producers_in_task;
	drake_schedule_task_t **schedule;
} drake_schedule_t;

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

#endif
