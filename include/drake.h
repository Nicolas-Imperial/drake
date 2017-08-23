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
//#include <drake/schedule.h>
//#include <drake/stream.h>
#include <drake/node.h>

#ifdef __cplusplus
#define drake_declare(application) \
extern "C" int drake_application_create(application)(); \
extern "C" int drake_application_init(application)(void*); \
extern "C" int drake_application_run(application)(); \
extern "C" int drake_application_destroy(application)(); //\
extern "C" int drake_task_number(application)(); \
extern "C" char* drake_task_name(application)(size_t);
#else
#define drake_declare(application) \
int drake_application_create(application)(); \
int drake_application_init(application)(void*); \
int drake_application_run(application)(); \
int drake_application_destroy(application)(); //\
int drake_task_number(application)(); \
char* drake_task_name(application)(size_t);
#endif
