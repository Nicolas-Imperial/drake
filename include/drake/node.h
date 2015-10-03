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


// Standard C includes
#include <stddef.h>

// Drake includes
#include <pelib/template.h>

// Drake includes
#include <drake/task.h>

#if PELIB_CONCAT_3(DONE_, TASK_MODULE, TASK_NAME) == 0

#ifdef _DRAKE_COMPILE

#define drake_init PELIB_CONCAT_3(TASK_MODULE,_init_,TASK_NAME)
#define drake_start PELIB_CONCAT_3(TASK_MODULE,_start_,TASK_NAME)
#define drake_run PELIB_CONCAT_3(TASK_MODULE,_run_,TASK_NAME)
#define drake_kill PELIB_##CONCAT_3(TASK_MODULE, _kill_, TASK_NAME)
#define drake_destroy PELIB_##CONCAT_3(TASK_MODULE, _destroy_, TASK_NAME)
#define drake_user(name) PELIB_##CONCAT_5(TASK_MODULE, _user_, TASK_NAME, _, name)

/** Task initialization function (as viewed from the streaming application), suitable for optional memory allocation or other slow, but unique operations. Runs before the corresponding stream begins to run
	@param task Task being initialized
	@ return 0 if there was an error in initialization
**/
int drake_init(task_t *, void *aux);
/** Task first work function (as viewed from the streaming application). Runs after the corresponding stream started to run
	@param task Task that begins to work
	@ return 0 if start function must run again before running the main work function
**/
int drake_start(task_t *);
/** Main work function of a task (as viewed from the streaming application)
	@param task Task at work
	@return 0 if the task must perform more work **/
int drake_run(task_t *);
/** Runs after a task stops working, before the corresponding stream stops. Function prototype as viewed from the streaming application
	@param task Task being killed
	@return 0 if the function encountered an error
**/
int drake_kill(task_t *);
/** Finalize the work of a task, deallocate optional memory allocated at initialization. Function prototype as viewed from the streaming application
	@param task Task being destroyed
	@return 0 if the function encountered an error
**/
int drake_destroy(task_t *);
#else
#define drake_init(name,id) PELIB_CONCAT_3(name,_init_,id)
#define drake_start(name,id) PELIB_CONCAT_3(name,_start_,id)
#define drake_run(name,id) PELIB_CONCAT_3(name,_run_,id)
#define drake_kill(name,id) PELIB_CONCAT_3(name,_kill_,id)
#define drake_destroy(name,id) PELIB_CONCAT_3(name,_destroy_,id)

/** Task initialization function (as viewed from the module compiler), suitable for optional memory allocation or other slow, but unique operations. Runs before the corresponding stream begins to run
	@param task Task being initialized
	@ return 0 if there was an error in initialization
**/
int drake_init(TASK_MODULE, TASK_NAME)(task_t *, void *aux);
/** Task first work function  (as viewed from the module compiler). Runs after the corresponding stream started to run
	@param task Task that begins to work
	@ return 0 if start function must run again before running the main work function
**/
int drake_start(TASK_MODULE, TASK_NAME)(task_t *);
/** Main work function of a task  (as viewed from the module compiler)
	@param task Task at work
	@return 0 if the task must perform more work **/
int drake_run(TASK_MODULE, TASK_NAME)(task_t *);
/** Runs after a task stops working, before the corresponding stream stops. Function prototype as viewed from the module compiler
	@param task Task being killed
	@return 0 if the function encountered an error
**/
int drake_kill(TASK_MODULE, TASK_NAME)(task_t *);
/** Finalize the work of a task, deallocate optional memory allocated at initialization. Function prototype as viewed from the module compiler
	@param task Task being destroyed
	@return 0 if the function encountered an error
**/
int drake_destroy(TASK_MODULE, TASK_NAME)(task_t *);

#undef TASK_MODULE
#undef TASK_NAME
#endif

#endif

