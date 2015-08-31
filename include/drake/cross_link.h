/*
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
#include <pelib/structure.h>
#define DONE_cross_link_tp 1

#define ARRAY_T cross_link_tp
#include <pelib/array.h>
#define DONE_array_cross_link_tp 1

#endif
