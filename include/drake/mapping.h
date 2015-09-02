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
#include <drake/processor.h>

#ifndef MAPPING_H
#define MAPPING_H

#define DRAKE_MAPPING_OUT_WARMUP "x [*,*]\n"
#define DRAKE_MAPPING_OUT_NODE_COLUMN_WIDTH 6
#define DRAKE_MAPPING_OUT_PROC_COLUMN_WIDTH 4
#define DRAKE_MAPPING_OUT_AFFECT " :="
#define DRAKE_MAPPING_OUT_ENDING ";"
struct mapping
{
	/// Number of processors involved in this mapping. Never modify this value manually
	unsigned int processor_count;
	/// Maximum number of processors this mapping can handle. This is the size of the processor array. Never modify this value manually
	unsigned int max_processor_count;
	/// Number of tasks mapped in this mapping. Never modify this value manually
	unsigned int task_count;
	/// List of processors involved in the mapping
	struct processor ** proc;
};
typedef struct mapping mapping_t;

#define STRUCT_T mapping_t
#include <pelib/structure.h>
#define DONE_mapping_t 1

/// Insert a new task to this mapping
int
drake_mapping_insert_task(mapping_t*, processor_id, task_t*);
/// Removes a task from this mapping
int
drake_mapping_remove_task(mapping_t*, task_id);
/// Finds the index of processor in the mapping processor array, from the processor's ID
int
drake_mapping_find_processor_index(mapping_t*, processor_id);
/// Finds the processor (its processor_id) which handles the task pointed by the task_id parameter
processor_id
drake_mapping_find_processor(mapping_t*, task_id);
/// Inserts a new processor in the mapping.
int
drake_mapping_insert_processor(mapping_t*, processor_t*);
/// Not implemented. Removes a processor from the mapping.
int
drake_mapping_remove_processor(mapping_t*, int);

int
drake_mapping_violations(mapping_t*);

char*
drake_mapping_drawstr(mapping_t*, char*);
/// Draws the mapping to a .fig file in the appropriate format
int
drake_mapping_draw(mapping_t*, FILE*);
/// Reads a mapping from the output of the ILP solver

/// Initializes a mapping_t structure and fills it with a mapping file obtained from the ILP solver
/// the parameter mapping should not be initialized, and the same pointer should receive the result of this function
/// \param mapping Mapping that will point to the data read
/// \param ilp_mapping_file File from where to read the mapping calculated by the ilp solver
mapping_t*
drake_mapping_loadfile(mapping_t* mapping, FILE* ilp_mapping_file);
/// Reads a mapping from a string in the same format than produced by ilp solver
mapping_t*
drake_mapping_loadstr(mapping_t*, char*);

/// Reads a mapping from a string similarly to drake_mapping_readfile(), and filter tasks according to filter function
/// \param mapping Mapping that will point to the data read
/// \param str Mapping string obtained from the ILP solver
/// \param filter Pointer to a user function that returns 0 if the task should be kept, non zero otherwise.
mapping_t*
drake_mapping_loadfilterstr(mapping_t* mapping, char* str, int (filter)(task_t*));

/// Similar to drake_mapping_readfile, but filter tasks according to a given user filter function
/// \param mapping Mapping that will point to the data read
/// \param ilp_mapping_file File from where to read the mapping calculated by the ilp solver
/// \param filter Pointer to a user function that returns 0 if the task should be kept, non zero otherwise.
mapping_t*
drake_mapping_loadfilterfile(mapping_t* mapping, FILE* ilp_mapping_file, int
( filter)(task_t*));

/// Find a task in a mapping, using its task id
/// \param mapping mapping to search in
/// \param id Id of the task to find
task_t*
drake_mapping_find_task(mapping_t* mapping, task_id id);

#endif
