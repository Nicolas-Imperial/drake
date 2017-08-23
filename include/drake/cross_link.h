#if 0
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

#ifndef CROSS_LINK_BUFFER
#define CROSS_LINK_BUFFER void
#endif

#ifndef CROSS_LINK_H
#define CROSS_LINK_H

/** Models a link between two tasks mapped to different processors **/
struct cross_link
{
	/// Pointer to the corresponding generic link
	link_t *link;
	/// State of the producer task of this link
	task_status_t *prod_state;
	/// Total number of elements read through this link, sent back from the consumer task
	volatile size_t *read;
	/// Total number of elements written through this link, sent by the producer along with data
	volatile size_t *write;
	/// Number of elements available in queue
	size_t available;
	/// Total number of read and write operations performed on fifo
	size_t total_read, total_written;
	/// Link to buffer that holds data being transmitted
	volatile CROSS_LINK_BUFFER *buffer;
	/// Actual number of elements 
	size_t actual_read, actual_written;
};
/** Space-less type alias for struct cross_link **/
typedef struct cross_link cross_link_t;
/** Symbol-less type alias for pointers to struct cross_link **/
typedef cross_link_t* cross_link_tp;

/** Define a cross link as a pelib object **/
#define STRUCT_T cross_link_tp
#include <pelib/structure.h>
#define DONE_cross_link_tp 1

/** Define array for cross links **/
#define ARRAY_T cross_link_tp
#include <pelib/array.h>
#define DONE_array_cross_link_tp 1

#endif
#endif
