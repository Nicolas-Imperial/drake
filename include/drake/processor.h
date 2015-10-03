/*
 Copyright 2015 Nicolas Melot

 This file is part of Drake.

 Drake is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Drake is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Drake. If not, see <http://www.gnu.org/licenses/>.

*/


#include <stddef.h>
#include <drake/task.h>
#include <drake/cross_link.h>

#ifndef PROCESSOR_H
#define PROCESSOR_H

#define DRAKE_MAPPING_PROC_ID_CHAR_LENGTH 2
#define DRAKE_MAPPING_NODE_CHAR_LENGTH 2
#define DRAKE_MAPPING_SEPARATOR ':'

/** Internal id of a processor within the drake framework **/
typedef unsigned int processor_id;

struct processor
{
	// Number of tasks assigned to this processor. Never modify this value manually
	unsigned int handled_nodes;
	/// Maximal number of tasks this processor can handle. This is related to the size of the task list in this structure. Never modify this value manually
	unsigned int node_capacity;
	/// Identification of the processor. All processors (say n processors) should be numbered from 0 to n-1 to be friendly with files generated to ILP solver and task graph graphic generator.
	processor_id id;
	/// List of cross links of tasks mapped to this core whose producer task is mapped to another core.
	array_t(cross_link_tp) *source;
	/// List of cross links of tasks mapped to this core whose consumer task is mapped to another core.
	array_t(cross_link_tp) *sink;
	/// Number of links between tasks mapped to this core
	int inner_links;
	/// List of tasks mapped to this core
	task_t ** task;
};
/** Space-less type alias for struct processor **/
typedef struct processor processor_t;

/** Inserts a copy of a task into the processor **/
int drake_processor_insert_task(processor_t *proc, task_t *task);
/** Removes a task from a processor **/
int drake_processor_remove_task(processor_t *proc, task_id task);
/** Finds the index of a task (starting at 0) within the list of task contained in a processor. If the task could not be found, return the number of tasks mapped to the processor. **/
size_t drake_processor_find_task(processor_t *proc, task_id task);

/// Generates a processor pelib object 
#define STRUCT_T processor_t
#include <pelib/structure.h>
#define DONE_processor_t 1

#endif // PROCESSOR_H
