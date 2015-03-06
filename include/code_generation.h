#ifndef CODE_GENERATION_H
#define CODE_GENERATION_H
#include "schedule_handling.h"
#include "taskgraph_handling.h"

int generate_code(schedule_t *schedule,taskgraph_t *taskgraph, const char *file_path, double walltime);



#endif
