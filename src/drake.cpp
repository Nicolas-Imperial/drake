#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iomanip>

#include <pelib/AmplInput.hpp>
#include <pelib/AmplOutput.hpp>
#include <pelib/Platform.hpp>
#include <pelib/GraphML.hpp>

#include <crown/allocation.h>
#include <crown/mapping.h>
#include <crown/frequency.h>
#include <crown/annealing.h>
#include <crown/crown.h>
#include <crown/CrownUtil.hpp>

#include <pelib/DrakeCSchedule.hpp>
#include <pelib/AmplInput.hpp>
#include <pelib/AmplOutput.hpp>
#include <pelib/Vector.hpp>
#include <pelib/Matrix.hpp>
#include <pelib/Set.hpp>
#include <pelib/GraphML.hpp>
#include <pelib/Schedule.hpp>

using namespace std;
using namespace pelib;
using namespace pelib::crown;

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
			while(*argv != '\0')
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
			ss << "bash -c '" << scheduler << " " << input_file << " " << platform_filename << " >(cat >" << (output_taskgraph == NULL ? "/dev/null" : output_taskgraph) << ") >(cat >" << (output_schedule == NULL ? "/dev/stdout" : output_schedule) << ") >/dev/null'";
			system(ss.str().c_str());

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
