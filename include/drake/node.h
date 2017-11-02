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
#include <drake/application.h>

#if PELIB_CONCAT_3(DONE_, TASK_MODULE, TASK_NAME) == 0

#ifdef _DRAKE_COMPILE

#define drake_init PELIB_CONCAT_3(TASK_MODULE,_init_,TASK_NAME)
#define drake_start PELIB_CONCAT_3(TASK_MODULE,_start_,TASK_NAME)
#define drake_run PELIB_CONCAT_3(TASK_MODULE,_run_,TASK_NAME)
#define drake_kill PELIB_##CONCAT_3(TASK_MODULE, _kill_, TASK_NAME)
#define drake_destroy PELIB_##CONCAT_3(TASK_MODULE, _destroy_, TASK_NAME)
#define drake_task PELIB_##CONCAT_3(TASK_MODULE, _task_, TASK_NAME)
#define drake_task_name() drake_task_get_name(drake_task())
#define drake_task_autokill() drake_autokill_task(drake_task())
#define drake_task_autosleep() drake_autosleep_task(drake_task())
#define drake_task_autoexit() drake_autoexit_task(drake_task())
#define drake_input_capacity(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _input_capacity_, link)()
#define drake_input_available(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _input_available_, link)()
#define drake_input_available_continuous(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _input_available_continuous_, link)()
#define drake_input_buffer(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _input_buffer_, link)
#define drake_input_discard(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _discard_, link)
#define drake_input_depleted(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _depleted_, link)()
#define drake_declare_input(name, type) \
size_t drake_input_capacity(name); \
size_t drake_input_available(name); \
size_t drake_input_available_continuous(name); \
type* drake_input_buffer(name)(size_t skip, size_t *size, type **extra); \
void drake_input_discard(name)(size_t size); \
int drake_input_depleted(name);
#define drake_output_capacity(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _output_capacity_, link)()
#define drake_output_available(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _output_available_, link)()
#define drake_output_available_continuous(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _output_available_continuous_, link)()
#define drake_output_buffer(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _output_buffer_, link)
#define drake_output_commit(link) PELIB_##CONCAT_5(TASK_MODULE, _link_, TASK_NAME, _commit_, link)
#define drake_task_core_id PELIB_##CONCAT_3(TASK_MODULE, _core_, TASK_NAME)
#define drake_task_instance PELIB_##CONCAT_3(TASK_MODULE, _instance_, TASK_NAME)
//#define drake_task_pool_create PELIB_##CONCAT_3(TASK_MODULE, _build_pool_, TASK_NAME)
#define drake_task_pool_run PELIB_##CONCAT_3(TASK_MODULE, _run_pool_, TASK_NAME)
#define drake_task_width() drake_task_get_width(drake_task())
//#define drake_task_pool_destroy PELIB_##CONCAT_3(TASK_MODULE, _destroy_pool_, TASK_NAME)
#define drake_declare_output(name, type) \
size_t drake_output_capacity(name); \
size_t drake_output_available(name); \
size_t drake_output_available_continuous(name); \
type* drake_output_buffer(name)(size_t *size, type **extra); \
void drake_output_commit(name)(size_t size);

/** Task initialization function (as viewed from the streaming application), suitable for optional memory allocation or other slow, but unique operations. Runs before the corresponding stream begins to run
	@param task Task being initialized
	@ return 0 if there was an error in initialization
**/
int drake_init(void *aux);
/** Task first work function (as viewed from the streaming application). Runs after the corresponding stream started to run
	@param task Task that begins to work
	@ return 0 if start function must run again before running the main work function
**/
int drake_start();
/** Main work function of a task (as viewed from the streaming application)
	@param task Task at work
	@return 0 if the task must perform more work **/
int drake_run();
/** Runs after a task stops working, before the corresponding stream stops. Function prototype as viewed from the streaming application
	@param task Task being killed
	@return 0 if the function encountered an error
**/
int drake_kill();
/** Finalize the work of a task, deallocate optional memory allocated at initialization. Function prototype as viewed from the streaming application
	@param task Task being destroyed
	@return 0 if the function encountered an error
**/
int drake_destroy();
/** Returns a task descriptor **/
drake_task_tp drake_task();

unsigned int
drake_task_core_id();

unsigned int
drake_task_instance();

int drake_task_pool_run(int (*func)(void*), void*);
//int drake_task_pool_wait();

#else
#define drake_init(name,id) PELIB_CONCAT_3(name,_init_,id)
#define drake_start(name,id) PELIB_CONCAT_3(name,_start_,id)
#define drake_run(name,id) PELIB_CONCAT_3(name,_run_,id)
#define drake_kill(name,id) PELIB_CONCAT_3(name,_kill_,id)
#define drake_destroy(name,id) PELIB_CONCAT_3(name,_destroy_,id)
#define drake_task(name,id) PELIB_CONCAT_3(name,_task_,id)
#define drake_task_name(name, id) drake_task_get_name(drake_task(name, id)())
#define drake_task_width() drake_task_##get_width(drake_task())
#define drake_task_autokill drake_autokill_task
#define drake_task_autosleep drake_autosleep_task
#define drake_task_autoexit drake_autoexit_task

// About input and output communication buffers
#define drake_input_capacity(name, id, link) PELIB_CONCAT_5(name, _link_, id, _input_capacity_, link)
#define drake_input_available(name, id, link) PELIB_CONCAT_5(name, _link_, id, _input_available_, link)
#define drake_input_available_continuous(name, id, link) PELIB_CONCAT_5(name, _link_, id, _input_available_continuous_, link)
#define drake_input_buffer(name, id, link) PELIB_CONCAT_5(name, _link_, id, _input_buffer_, link)
#define drake_input_discard(name, id, link) PELIB_CONCAT_5(name, _link_, id, _discard_, link)
#define drake_input_depleted(name, id, link) PELIB_CONCAT_5(name, _link_, id, _depleted_, link)
#define drake_output_capacity(name, id, link) PELIB_CONCAT_5(name, _link_, id, _output_capacity_, link)
#define drake_output_available(name, id, link) PELIB_CONCAT_5(name, _link_, id, _output_available_, link)
#define drake_output_available_continuous(name, id, link) PELIB_CONCAT_5(name, _link_, id, _output_available_continuous_, link)
#define drake_output_buffer(name, id, link) PELIB_CONCAT_5(name, _link_, id, _output_buffer_, link)
#define drake_output_commit(name, id, link) PELIB_CONCAT_5(name, _link_, id, _commit_, link)
#define drake_task_core_id(name, id) PELIB_##CONCAT_3(name, _core_, id)
#define drake_task_instance(name, id) PELIB_##CONCAT_3(name, _instance_, id)
//#define drake_task_pool_create(name, id) PELIB_##CONCAT_3(name, _build_pool_, id)
#define drake_task_pool_run(name, id) PELIB_##CONCAT_3(name, _run_pool_, id)
//#define drake_task_pool_destroy(name, id) PELIB_##CONCAT_3(name, _destroy_pool_, id)

/** Task initialization function (as viewed from the module compiler), suitable for optional memory allocation or other slow, but unique operations. Runs before the corresponding stream begins to run
	@param task Task being initialized
	@ return 0 if there was an error in initialization
**/
int drake_init(TASK_MODULE, TASK_NAME)(void *aux);
/** Task first work function  (as viewed from the module compiler). Runs after the corresponding stream started to run
	@param task Task that begins to work
	@ return 0 if start function must run again before running the main work function
**/
int drake_start(TASK_MODULE, TASK_NAME)();
/** Main work function of a task  (as viewed from the module compiler)
	@param task Task at work
	@return 0 if the task must perform more work **/
int drake_run(TASK_MODULE, TASK_NAME)();
/** Runs after a task stops working, before the corresponding stream stops. Function prototype as viewed from the module compiler
	@param task Task being killed
	@return 0 if the function encountered an error
**/
int drake_kill(TASK_MODULE, TASK_NAME)();
/** Finalize the work of a task, deallocate optional memory allocated at initialization. Function prototype as viewed from the module compiler
	@param task Task being destroyed
	@return 0 if the function encountered an error
**/
int drake_destroy(TASK_MODULE, TASK_NAME)();

/** Returns a task descriptor **/
drake_task_tp drake_task(TASK_MODULE, TASK_NAME)();

unsigned int
drake_task_core_id(name, id)();

unsigned int
drake_task_instance(name, id)();

int drake_task_pool_run(name, id)(int (*func)(void*), void*);
//int drake_task_pool_wait(name, id)();

#undef TASK_MODULE
#undef TASK_NAME
#endif


#endif

