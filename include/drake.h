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
#include <drake/application.h>
#include <drake/node.h>

#ifdef __cplusplus
#define drake_declare(application) \
extern "C" struct drake_application* drake_application_build(application)(); \
extern "C" unsigned int drake_application_number_of_cores(application);
#else
#define drake_declare(application) \
struct drake_application* drake_application_build(application)(); \
unsigned int drake_application_number_of_cores(application);
#endif

