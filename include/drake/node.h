/*
 * node.h
 *
 *  Created on: 26 Jan 2012
 *  Copyright 2012 Nicolas Melot
 *
 * This file is part of node.
 * 
 *     drake is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     drake is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with drake. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

// Standard C includes
#include <stddef.h>

// Drake includes
#include <pelib/template.h>

// Drake includes
#include <drake/task.h>

#if DRAKE_CONCAT_3(DONE_, TASK_NAME, TASK_ID) == 0

#ifdef _DRAKE_COMPILE

#define drake_init DRAKE_CONCAT_3(TASK_NAME,_init_,TASK_ID)
#define drake_start DRAKE_CONCAT_3(TASK_NAME,_start_,TASK_ID)
#define drake_run DRAKE_CONCAT_3(TASK_NAME,_run_,TASK_ID)
#define drake_destroy DRAKE_##CONCAT_3(TASK_NAME, _destroy_, TASK_ID)
#define drake_user(name) DRAKE_##CONCAT_5(TASK_NAME, _user_, TASK_ID, _, name)

int drake_init(task_t *, void *aux);
int drake_start(task_t *);
int drake_run(task_t *);
int drake_destroy(task_t *);
#else
#define drake_init(name,id) DRAKE_CONCAT_3(name,_init_,id)
#define drake_start(name,id) DRAKE_CONCAT_3(name,_start_,id)
#define drake_run(name,id) DRAKE_CONCAT_3(name,_run_,id)
#define drake_destroy(name,id) DRAKE_CONCAT_3(name,_destroy_,id)

int drake_init(TASK_NAME, TASK_ID)(task_t *, void *aux);
int drake_start(TASK_NAME, TASK_ID)(task_t *);
int drake_run(TASK_NAME, TASK_ID)(task_t *);
int drake_destroy(TASK_NAME, TASK_ID)(task_t *);

#undef TASK_NAME
#undef TASK_ID
#endif

#endif

