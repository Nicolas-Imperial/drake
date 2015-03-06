/*
  Author: Johan Janzen

  This is the generic interface to the target system for the eval framework.
  I you want to port this framework, impelement these functions for your 
  target system.

 */
#ifndef TARGET_FUNCTIONS_H
#define TARGET_FUNCTIONS_H
#include "schedeval.h"
#include "taskgraph_handling.h"
#include "schedule_handling.h"
// Setup target system for execution
// When this function returns with value 0 the system is ready to go
int target_setup(int number_of_nodes);

// Compile the executable that will be run on the target system
// Assumes that the aut has already been compiled and is available as
// src/aut/[applicationname].a
int target_compile_exec(taskgraph_t *taskgraph,schedule_t *schedule);

// Writes the path to the target executable to "path", or rather, where it
// should be if it has ben compiled.
void path_to_exec(taskgraph_t *taskgraph,schedule_t *schedule, char* path);

// Create the aut archice file. target_compile_exec does this too,so this is for
// debugging convenicene. 
int target_compile_aut(taskgraph_t *taskgraph);

// Performs the evaluation
int target_run(const char *path_to_exec,const char *args, schedule_t *schedule);

void fetch_print_powerdata(const char *path_to_exec);
#endif
