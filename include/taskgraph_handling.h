#ifndef TASKGRAPH_HANDLING_H
#define TASKGRAPH_HANDLING_H
extern "C"{
#include <igraph.h>
}
#include "schedule_handling.h"
typedef igraph_t taskgraph_t;

int taskgraph_load(const char *path, taskgraph_t **taskgraph);


void taskgraph_destroy(taskgraph_t *taskgraph);
//int generate_edges_array(taskgraph_t *taskgraph, schedule_t *schedule, FILE *file);
const char* taskgraph_getname(const taskgraph_t *taskgraph);
int number_of_nodes(const taskgraph_t *taskgraph);
void taskgraph_printinfo(const taskgraph_t *taskgraph);

size_t taskgraph_get_numberoftasks(const taskgraph_t *taskgraph);
const char *taskgraph_find_taskname(const taskgraph_t *taskgraph, const char *taskid);
const char *taskgraph_get_taskname(const taskgraph_t *taskgraph, size_t nodeid);
const char *taskgraph_get_taskid(const taskgraph_t *taskgraph, size_t nodeid);

#endif
