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
#include <pelib/integer.h>

#ifndef LINK_H
#define LINK_H

/** Forward declaration of pointer to tasks **/
typedef struct task* task_tp;

/// Generates task pointer pelib object
#define STRUCT_T task_tp
#include <pelib/structure.h>
#define DONE_task_tp 1

/// Generates arrays for task pointer objects
#define ARRAY_T task_tp
#include <pelib/array.h>
#define DONE_array_task_tp 1

#ifndef INNER_LINK_BUFFER
#define INNER_LINK_BUFFER void
#endif

/** Generic link between tasks, regardless of their mapping **/
struct link
{
	/// Consumer task of this link
	struct task *prod;
	/// Producer task of this link
	struct task *cons;
	//char *name;
	/// Fifo buffer that holds data transmitted between both tasks
	cfifo_t(int)* buffer;
};
/// Space-less type alias for struct link
typedef struct link link_t;
/// Symbol-less type alias for pointer to struct link
typedef link_t* link_tp;

/// Generates link pointer pelib object
#define STRUCT_T link_tp
#include <pelib/structure.h>
#define DONE_link_tp 1

/// Generates arrays for link pointer objects
#define ARRAY_T link_tp
#include <pelib/array.h>
#define DONE_array_link_tp 1

#endif // LINK_T
#endif
