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

#include <drake/platform.h>

#ifndef TASK_H
#define TASK_H

#ifdef __cplusplus
extern "C"
{
#endif

struct drake_task_pool
{
	int state;
	drake_local_barrier_t barrier;
	int (*work)(void *);
	void *aux;
};

void drake_task_pool_create(struct drake_task_pool *pool);
int drake_task_pool_dorun(struct drake_task_pool *pool, int (*func)(void*), void* aux);
int drake_task_pool_destroy(struct drake_task_pool *pool);

#ifdef __cplusplus
}
#endif

#endif
