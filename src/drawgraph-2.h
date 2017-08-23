#if 0
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


#include <drake/mapping.h>

#ifndef DRAWGRAPH2_H_
#define DRAWGRAPH2_H_

#define PMAX 11   // do not use proc 0
#define NMAX 1025 // do not use node 0

#define DRAKE_MAPPING_MAX_PROCESSOR_COUNT 16
#define DRAKE_MAPPING_MAX_TASK_COUNT    256

/**  Generates graphic mapping representation from a mapping AMPL file
	@param input Input matrix, task id (1 to n) vs processor (1 to p) in AMPL output format. 1 if task M(n,p) == 1 iff task n is mapped to processor p
	@param output Output image file
	@param error Output stream where error messages are written
**/
void
pelib_drawgraph2_draw(FILE* input, FILE* output, FILE* error);
mapping_t*
/** Reads a AMPL mapping matrix (task vs processor) and returns a corresponding instance if mapping_t **/
pelib_drawgraph2_load(FILE*, mapping_t*, int(filter)(task_t*));
#endif /* DRAWGRAPH2_H_ */
#endif
