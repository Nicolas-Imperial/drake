#include <stddef.h>
#include <task.h>

#ifndef CROSS_LINK_BUFFER
#define CROSS_LINK_BUFFER void
#endif

#ifndef CROSS_LINK_H
#define CROSS_LINK_H

struct cross_link
{
	link_t *link;
	task_status_t *prod_state;
	volatile size_t *read;
	volatile size_t *write;
	size_t available;
	size_t total_read, total_written;
	volatile CROSS_LINK_BUFFER *buffer;
	size_t actual_read, actual_written;
};
typedef struct cross_link cross_link_t;
typedef cross_link_t* cross_link_tp;

#define STRUCT_T cross_link_tp
#include <structure.h>
#define DONE_cross_link_tp 1

#define ARRAY_T cross_link_tp
#include <array.h>
#define DONE_array_cross_link_tp 1

#endif
