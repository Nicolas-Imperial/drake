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

#include <stdlib.h>

#include <drake/task.h>

#if 1 
#define debug(var) printf("[%s:%s:%d:P%u] %s = \"%s\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_addr(var) printf("[%s:%s:%d:P%u] %s = \"%p\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_int(var) printf("[%s:%s:%d:P%u] %s = \"%d\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#define debug_size_t(var) printf("[%s:%s:%d:P%u] %s = \"%zu\"\n", __FILE__, __FUNCTION__, __LINE__, drake_platform_core_id(), #var, var); fflush(NULL)
#else
#define debug(var)
#define debug_addr(var)
#define debug_int(var)
#define debug_size_t(var)
#endif

void
drake_task_pool_create(struct drake_task_pool *pool)
{
	// Run by all cores but the master core of some parallel task
	// The following points are referred to in comments in related
	// other functions below

	// Point CreateA
	// This barrier prevents non-master cores from starting to work
	// before the work function was defined.
	drake_platform_local_barrier_wait(pool->barrier);

	while(pool->work != NULL)
	{
		// Point CreateB
		pool->work(pool->aux);

		// Point CreateC
		// Waits for all cores to be done working before
		// the master core can resume its lone job
		drake_platform_local_barrier_wait(pool->barrier);

		// Point CreateD
		// Wait for a new order, work or leave, set by the
		// master core. If pool->work is null, then no work
		// will have to be performed anymore and the pool
		// is destroyed.
		drake_platform_local_barrier_wait(pool->barrier);
	}	

	// Point CreateE
	// Prevent master core from setting pool->work
	// to non-null before non-master cores get a chance
	// to evaluate the if() pool->statement above.
	drake_platform_local_barrier_wait(pool->barrier);

	// Point CreateF
}

int
drake_task_pool_destroy(struct drake_task_pool *pool)
{
	// Important, see comments below
	pool->work = NULL;

	// Two possible cases
	// (a) The pool has never been run: all non-master cores are at
	// point CreateA
	// (b) The pool has been run at least once: all non-master cores are
	// at point CreateD

	drake_platform_local_barrier_wait(pool->barrier);

	// In case (a), non-master cores skip the loop because pool->work is
	// set to NULL. They reach point CreateE and exit to next task. In
	// case (b), They exit the loop because pool->work == NULL, reach point
	// CreateE and also exit to next task to execute.

	drake_platform_local_barrier_wait(pool->barrier);

	// Non-master threads were at point CreateE, they will now transition
	// to point CreateF and destroy the thread pool

	//drake_platform_local_barrier_wait(pool->barrier);

	return 1;
}

int 
drake_task_pool_dorun(struct drake_task_pool *pool, int (*func)(void*), void* aux)
{
	pool->work = func;
	pool->aux = aux;

	// We assume here that pool->work != NULL, i.e., it points to an
	// executable function by all cores participating to the calling task

	// Two possible cases:
	// (a) This is the first time the pool runs within a task instance of
	// a pipeline stage.
	// (b) This is the second or more time the pool runs some job
	// If we are in (a), then all non-master cores are at point CreateA
	// Otherwise (case (b)) all non-master cores are at point CreateD

	drake_platform_local_barrier_wait(pool->barrier);
	
	// In case (a), non-master cores cross the barrier and enter the if
	// clause above to reach CreateB, because pool->work != NULL
	// In case (b), non-master cores cross the barrier and execute another
	// instance of the loop to reach point CreateB
	// Consequently, all non-master cores are garanteed to reach point
	// CreateB at this point

	int res = func(aux);
	// Because all non-master cores were garanteed to be at point CreateB,
	// They are all garanteed to eventually reach point CreateC (assuming
	// pool->work eventually terminates).

	drake_platform_local_barrier_wait(pool->barrier);
	// Because all threads were at point createC, they are now in createD
	// Allowing either an other round of work (new call to drake_task_pool_dorun())
	// or to exit (new call to drake_task_pool_destroy()) by the master core

	return res;
}

