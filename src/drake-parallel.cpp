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


#include <iostream>
#include <cstdlib>
#include <vector>
#include <map>

#include <pelib/AmplInput.hpp>
#include <pelib/Taskgraph.hpp>
#include <pelib/Platform.hpp>
#include <pelib/Schedule.hpp>

#include <pelib/XMLSchedule.hpp>

#include <pelib/process.h>
#include <pelib/argument_parsing.hpp>
#include <pelib/dl.h>

#ifdef debug
#undef debug
#endif

#define debug(expr) cerr << "[" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "] " << #expr << " = \"" << expr << "\"." << endl;

using namespace std;
using namespace pelib;

#ifdef __cplusplus
extern "C" {
#endif

struct args
{
	bool showStatsOut;
	bool showStatsErr;
	bool showDescription;
	char* showStatsFile;
	pelib_argument_stream lib;
};
typedef struct args args_t;

static
args_t
parse(char** arg)
{
	args_t args;
	args.showStatsOut = false;
	args.showStatsErr = false;
	args.showStatsFile = NULL;
	args.showDescription = false;
	pelib_argument_stream_init(&args.lib);

	for(; arg[0] != NULL && string(arg[0]).compare("--") != 0; arg++)
	{
		if(strcmp(arg[0], "--show-stats") == 0)
		{
			arg++;
			
			if(strcmp(arg[0], "--stdout") == 0)
			{
				args.showStatsOut = true;
			}
			else if(strcmp(arg[0], "--stderr") == 0)
			{
				args.showStatsErr = true;
			}
			else
			{
				args.showStatsFile = arg[0];
			}
			continue;
		}
		if(strcmp(arg[0], "--description") == 0)
		{
			args.showDescription = true;
			continue;
		}

		// Nothing else, try to parse a library
		arg += pelib_argument_stream_parse(arg, &args.lib) - 1;
		continue;
	}

	return args;
}

// TODO: find a way to use libdl to invoke the library that created the object
char *library;

std::map<string, pelib::Record*>
pelib_process(const std::map<string, pelib::Record*> &records, size_t argc, char** argv)
{
	// Parse extra arguments and skip them
	args_t args = parse(argv);

	if(args.lib.library == NULL)
	{
		cerr << "[ERROR] No scheduler specified. Aborting" << endl;
		return map<string, Record*>();
	}

	// Load library
	library = args.lib.library;
	void *libScheduler = load_lib(library);
	map<string, Record*> output;

	if(!args.showDescription)
	{
		Schedule* (*schedule)(const Taskgraph &tg, const Platform &pt, size_t argc, char **argv, map<const string, double> &statistics) = (Schedule* (*)(const Taskgraph &tg, const Platform &pt, size_t argc, char **argv, map<const string, double> &statistics))load_function(libScheduler, "pelib_schedule");

		// Read input data
		if(records.find(typeid(Taskgraph).name()) != records.end() && records.find(typeid(Platform).name()) != records.end())
		{
			Taskgraph *tg = (Taskgraph*)records.find(typeid(Taskgraph).name())->second;
			if(tg == NULL)
			{
				cerr << "[ERROR] Missing taskgraph description. Aborting" << endl;
				return map<string, Record*>();
			}	
			Platform *pt = (Platform*)records.find(typeid(Platform).name())->second;
			if(pt == NULL)
			{
				cerr << "[ERROR] Missing platform description. Aborting" << endl;
				return map<string, Record*>();
			}

			// Prepare output collection and fill it with schedule generated by library
			map<const string, double> statistics;
			Schedule *sched = schedule(*tg, *pt, args.lib.argc, args.lib.argv, statistics);
			//Schedule *sched = new Schedule("Dummy", "Dummy", Schedule::table());
			Taskgraph *ptg = new Taskgraph(tg);
			Platform *ppt = new Platform(pt);
			output.insert(pair<string, Record*>(typeid(Schedule).name(), sched));
			output.insert(pair<string, Record*>(typeid(Taskgraph).name(), ptg));
			output.insert(pair<string, Record*>(typeid(Platform).name(), ppt));

			// Show solving statistics
			if(args.showStatsFile != NULL)
			{
				ofstream statsFileStr(args.showStatsFile);
				for(map<const string, double>::iterator i = statistics.begin(); i != statistics.end(); i++)
				{
					statsFileStr << i->first << " = " << i->second << endl;
				}
				statsFileStr.close();
			}
			else if(args.showStatsOut || args.showStatsErr)
			{
				for(map<const string, double>::iterator i = statistics.begin(); i != statistics.end(); i++)
				{
					if(args.showStatsOut)
					{
						cout << i->first << " = " << i->second << endl;
					}
					else
					{
						cerr << i->first << " = " << i->second << endl;
					}
				}
			}
		}
	}
	else
	{
		string (*description)(size_t argc, char **argv) = (string (*)(size_t argc, char **argv))load_function(libScheduler, "pelib_description");
		cout << description(args.lib.argc, args.lib.argv) << endl;
	}

	destroy_lib(libScheduler);

	return output;
}

void
pelib_delete(pelib::Record* obj)
{
	void *libScheduler = load_lib(library);
	void (*del)(const Schedule*) = (void (*)(const Schedule*))load_function(libScheduler, "pelib_delete");

	if(string(typeid(Schedule).name()).compare(typeid(*obj).name()) == 0)
	{
		del((Schedule*)obj);
	}
	else
	{
		delete obj;
	}
}

#ifdef __cplusplus
}
#endif

