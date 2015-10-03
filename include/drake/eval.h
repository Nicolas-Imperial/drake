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


#include <drake/platform.h>

#ifndef DRAKE_EVAL
#define DRAKE_EVAL

/** Arguments passed to a drake streaming application under evaluation **/
typedef struct args
{
	size_t argc;
	char **argv;
} args_t;

// pthread *hates* when a pointer is named "kill".
/** Pointers to time structures recording time when the corresponding task function runs **/
drake_time_t *init, *start, *run, *killed, *destroy;

/** Array of boolean private for each processor. Takes value 1 if the processor runs the task at all and 0 otherwise **/
int *execute;

#endif
