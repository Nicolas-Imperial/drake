/*
 * test.c
 *
 *  Created on: 13 Jan 2012
 *  Copyright 2012 Nicolas Melot
 *
 * This file is part of ccpp.
 * 
 *     ccpp is free software: you can redistribute it and/or modify
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <unitc.h>

void
test_setup()
{
	// do nothing
}

void
test_teardown()
{
	// do nothing
}

void
test_init()
{
	// do nothing
}

void
test_cleanup()
{
	// do nothing
}

static void
dummy()
{
	assert(1);
}

void
test_run()
{
	test(dummy);
}
