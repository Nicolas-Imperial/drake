/*
 * drawgraph-2.h
 *
 *  Created on: Feb 8, 2011
 *  Copyright 2011 Nicolas Melot
 *
 * This file is part of pelib.
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

#include <snekkja/mapping.h>

#ifndef DRAWGRAPH2_H_
#define DRAWGRAPH2_H_

#define PMAX 11   // do not use proc 0
#define NMAX 1025 // do not use node 0

#define PELIB_MAPPING_MAX_PROCESSOR_COUNT 16
#define PELIB_MAPPING_MAX_TASK_COUNT    256

void
pelib_drawgraph2_draw(FILE*, FILE*, FILE*);
mapping_t*
pelib_drawgraph2_load(FILE*, mapping_t*, int(filter)(task_t*));
#endif /* DRAWGRAPH2_H_ */
