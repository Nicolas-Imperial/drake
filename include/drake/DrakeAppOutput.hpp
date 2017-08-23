/*
 Copyright 2015 Nicolas Melot

 This file is part of Pelib.

 Pelib is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Pelib is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Pelib. If not, see <http://www.gnu.org/licenses/>.
*/


#include <pelib/Schedule.hpp>

#include <pelib/ScheduleOutput.hpp>

#ifndef PELIB_DRAKECSCHEDULEOUTPUT_H
#define PELIB_DRAKECSCHEDULEOUTPUT_H

namespace drake
{
	/** Base class for any class that can generate C code that represents an instance of drake::Schedule with all its associated Taskgraph and Platform details **/
	class DrakeAppOutput : public pelib::ScheduleOutput
	{
	};
}

#endif
