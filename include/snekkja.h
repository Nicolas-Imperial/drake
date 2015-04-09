/*
 * snekkja.h
 *
 *  Created on: 26 Jan 2012
 *  Copyright 2012 Nicolas Melot
 *
 * This file is part of snekkja.
 * 
 *     pelib is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 * 
 *     pelib is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 * 
 *     You should have received a copy of the GNU General Public License
 *     along with pelib. If not, see <http://www.gnu.org/licenses/>.
 * 
 */

// Standard C includes
#include <stddef.h>

// Pelib includes
#include <template.h>

// Snekkja includes
#include <snekkja/task.h>

#if PELIB_CONCAT_3(DONE_, TASK_NAME, TASK_ID) == 0

#ifdef _SNEKKJA_COMPILE

#define snekkja_init PELIB_CONCAT_3(TASK_NAME,_init_,TASK_ID)
#define snekkja_start PELIB_CONCAT_3(TASK_NAME,_start_,TASK_ID)
#define snekkja_run PELIB_CONCAT_3(TASK_NAME,_run_,TASK_ID)
#define snekkja_destroy PELIB_##CONCAT_3(TASK_NAME, _destroy_, TASK_ID)
#define snekkja_user(name) PELIB_##CONCAT_5(TASK_NAME, _user_, TASK_ID, _, name)

int snekkja_init(task_t *, void *aux);
int snekkja_start(task_t *);
int snekkja_run(task_t *);
int snekkja_destroy(task_t *);
#else
#define snekkja_init(name,id) PELIB_CONCAT_3(name,_init_,id)
#define snekkja_start(name,id) PELIB_CONCAT_3(name,_start_,id)
#define snekkja_run(name,id) PELIB_CONCAT_3(name,_run_,id)
#define snekkja_destroy(name,id) PELIB_CONCAT_3(name,_destroy_,id)

int snekkja_init(TASK_NAME, TASK_ID)(task_t *, void *aux);
int snekkja_start(TASK_NAME, TASK_ID)(task_t *);
int snekkja_run(TASK_NAME, TASK_ID)(task_t *);
int snekkja_destroy(TASK_NAME, TASK_ID)(task_t *);

#undef TASK_NAME
#undef TASK_ID
#endif

#endif

