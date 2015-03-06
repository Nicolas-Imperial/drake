#ifndef SCHEDULE_HANDLING_H
#define SCHEDULE_HANDLING_H
extern "C"{
#include <libxml/parser.h>
}
//#include "taskgraph_handling.h"
typedef xmlDoc schedule_t;

char* schedule_getname(schedule_t *schedule);
int schedule_load(const char *path, schedule_t **schedule);
void schedule_destroy(schedule_t *schedule);
void schedule_printinfo(schedule_t *schedule);
//void schedule_generate_code(schedule_t *schedule, const taskgraph_t *taskgraph);
int schedule_find_task(schedule_t *schedule,const char *taskid, int *core, int *ordering);
int schedule_find_min_ordering(schedule_t *schedule);

int schedule_get_num_cores(schedule_t *schedule);
int schedule_get_num_tasks(schedule_t *schedule);

//int get_int_attr(xmlNode *node,const char* attrname);
//char get_str_attr(xmlNode *node,const char* attrname)

//typedef xmlNode* taskiterator_t;
//taskiterator_t schedule_init_taskiterator(schedule_t *schedule);
#endif
