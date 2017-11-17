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


#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iomanip>

#include <pelib/Platform.hpp>
#include <pelib/GraphML.hpp>
#include <pelib/Schedule.hpp>

using namespace std;
using namespace pelib;

enum action {NONE, NAME, TASKS, TASK, SCHEDULE};

enum action request = NONE;
int input_stdin = 0;
char* input_file = NULL;
char* task_name = NULL;
char *output_schedule = NULL;
char *output_taskgraph = NULL;

char *platform_filename = NULL, *parameters_filename = NULL;
char *scheduler;

#if DEBUG
#define trace(var) cerr << "[" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "] " << #var << " = \"" << var << "\"." << endl
#else
#define trace(var)
#endif

static void
set_action(enum action req)
{
	if(request != NONE)
	{
		cerr << "[WARN ] Action already set, replacing." << endl;
	}
	request = req;
}

static void
read_args(char **argv)
{
	while(*argv != NULL)
	{
//		cerr << "[DEBUG] argv = \"" << *argv << "\"." << endl;
//		cerr << "[DEBUG] strcmp(*argv, \"--tasks\") == \"" << strcmp(*argv, "--tasks") << "\"." << endl;
		if(!strcmp(*argv, "--schedule") || !strcmp(*argv, "-g"))
		{
				set_action(SCHEDULE);
		}
		if(!strcmp(*argv, "--name") || !strcmp(*argv, "-g"))
		{
				set_action(NAME);
		}
		if(!strcmp(*argv, "--tasks") || !strcmp(*argv, "-g"))
		{
				set_action(TASKS);
		}
		if(!strcmp(*argv, "--module") || !strcmp(*argv, "-g"))
		{
			set_action(TASK);
			argv++;
			if(**argv != '-')
			{
				task_name = *argv;
			}
			else
			{
				cerr << "[WARN ] Missing task name to read information from. Ignoring." << endl;
				argv--;
			}
		}
		else if(!strcmp(*argv, "--platform") || !strcmp(*argv, "-t"))
		{
			// Read next argument
			argv++;
			if(*argv[0] == '-')
			{	
				cerr << "[WARN ] Cannot read platform from standard input or invalid option for platform (\"" << argv << "\" starts with '-'). Ignoring." << endl;
				argv--;
			}
			else
			{
				platform_filename = *argv;
			}
		}
		else if(!strcmp(*argv, "--taskgraph") || !strcmp(*argv, "-t"))
		{
			// Get to next parameter
			argv++;

			if(*argv[0] == '-')
			{
				cerr << "[WARN ] Cannot read taskgraph from standard input or invalid option for taskgraph (\"" << argv << "\" starts with '-'). Ignoring." << endl;
			} else
			{
				input_file = *argv;
			}
		}
		else if(!strcmp(*argv, "--scheduler") || !strcmp(*argv, "-s"))
		{
			argv++;
			scheduler = *argv;
		}
		else if(!strcmp(*argv, "--output") || !strcmp(*argv, "-s"))
		{
			argv++;
			while(*argv != NULL)
			{
				if(!strcmp(*argv, "--schedule") || !strcmp(*argv, "-s"))
				{
					argv++;
					if(*argv[0] == '-')
					{
						cerr << "[WARN ] Invalid filename for output schedule (\"" << argv << "\" starts with '-'). Ignoring." << endl;
					}
					else
					{
						output_schedule = *argv;
					}
					argv++;
				}
				else if(!strcmp(*argv, "--taskgraph") || !strcmp(*argv, "-s"))
				{
					argv++;
					if(*argv[0] == '-')
					{
						cerr << "[WARN ] Invalid filename for output taskgraph (\"" << argv << "\" starts with '-'). Ignoring." << endl;
					}
					else
					{
						output_taskgraph = *argv;
					}
					argv++;
				}
				else
				{
					break;
				}
			}
		}

		// Read next argument
		argv++;
	}
}

int main(int argc, char **argv)
{
	read_args(argv);
	
	switch(request)
	{
		case NAME:
		{
			GraphML input;
			std::ifstream myfile;
			myfile.open (input_file, std::ios::in);
			Taskgraph tg = input.parse(myfile);
			myfile.close();

			cout << tg.getName() << endl;	
			break;
		}
		case TASKS:
		{
			GraphML input;
			std::ifstream myfile;
			myfile.open (input_file, std::ios::in);
			Taskgraph tg = input.parse(myfile);
			myfile.close();

			for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
			{
				cout << i->getName() << " ";
			}
			cout << endl;

			break;
		}
		case TASK:
		{
			GraphML input;
			std::ifstream myfile;
			myfile.open (input_file, std::ios::in);
			Taskgraph tg = input.parse(myfile);
			myfile.close();

			cout << tg.findTask(task_name).getModule() << endl;
			break;
		}
		case SCHEDULE:
		{	
			stringstream ss;
			// Run a mimer scheduler, discard the output taskgraph and statistics and redirect taskgraph and schedule outputs to respective output files (default: /dev/null for taskgraph and /dev/stdout for schedule)
			ss << "bash -c '" << scheduler << " --taskgraph " << input_file << " --platform " << platform_filename << " --taskgraph-output >(cat >" << (output_taskgraph == NULL ? "/dev/null" : output_taskgraph) << ") --schedule-output >(cat >" << (output_schedule == NULL ? "/dev/stdout" : output_schedule) << ") >/dev/null'";
			cerr << "Running \"" << ss.str() << "\"" << endl;
			int res = system(ss.str().c_str());

			if(res != 0)
			{
				cerr << "[ERROR] Error while running scheduler \"" << scheduler << "\". Aborting." << endl;
				exit(res);
			}

			break;
		}
		case NONE:
			cerr << "[ERROR] No valid action requested. Aborting." << endl;
			exit(1);
		break;
		default:
			cerr << "[ERROR] Unknown action requested. Aborting." << endl;
			exit(1);
		break;
	}
		
	// Set floating point var output format to fixed at 7 digits
	std::cout << std::setprecision(6)                                                                                                        
		<< std::setiosflags(std::ios::fixed)                                                                                                     
		<< std::setiosflags(std::ios::showpoint);  

	return 0;
}
