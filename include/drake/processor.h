#include <stddef.h>
#include <drake/task.h>
#include <drake/cross_link.h>

#ifndef PROCESSOR_H
#define PROCESSOR_H

#define DRAKE_MAPPING_PROC_ID_CHAR_LENGTH 2
#define DRAKE_MAPPING_NODE_CHAR_LENGTH 2
#define DRAKE_MAPPING_SEPARATOR ':'

typedef unsigned int processor_id;

struct processor
{
	// Number of tasks assigned to this processor. Never modify this value manually
	unsigned int handled_nodes;
	/// Maximal number of tasks this processor can handle. This is related to the size of the task list in this structure. Never modify this value manually
	unsigned int node_capacity;
	/// Identification of the processor. All processors (say n processors) should be numbered from 0 to n-1 to be friendly with files generated to ILP solver and task graph graphic generator.
	processor_id id;
	/// List of tasks the processor has to process
	array_t(cross_link_tp) *source;
	array_t(cross_link_tp) *sink;
//		volatile unsigned char *private_mem, *mpb;
//		int buffer_size;
	int inner_links;
//		input_ctrl_t *input_ctrl;
//		input_buffer_t input_buffer;
	// make sure to pad to 32 bytes from last member
//		output_ctrl_t *output_ctrl;
	// make sure to pad to 32 bytes from last member
	task_t * task[];
};
typedef struct processor processor_t;

int drake_processor_insert_task(processor_t *proc, task_t *task);
int drake_processor_remove_task(processor_t *proc, task_id task);
size_t drake_processor_find_task(processor_t *proc, task_id task);

#define STRUCT_T processor_t
#include <pelib/structure.h>
#define DONE_processor_t 1

#endif // PROCESSOR_H
