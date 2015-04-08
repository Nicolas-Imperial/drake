#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <iomanip>

#include <AmplInput.hpp>
#include <AmplOutput.hpp>
#include <Platform.hpp>
#include <AmplPlatform.hpp>
#include <GraphML.hpp>

#include <allocation.h>
#include <mapping.h>
#include <frequency.h>
#include <annealing.h>
#include <crown.h>

#include <SnekkjaCSchedule.hpp>
#include <CrownUtil.hpp>
#include <AmplInput.hpp>
#include <AmplOutput.hpp>
#include <Vector.hpp>
#include <Matrix.hpp>
#include <Set.hpp>
#include <GraphML.hpp>
#include <AmplPlatform.hpp>
#include <Schedule.hpp>

using namespace std;
using namespace pelib;
using namespace pelib::crown;

enum scheduler {CROWN_BINARY_ANNEALING_ALLOCATION_OFF};
enum action {NONE, NAME, TASKS, TASK, SCHEDULE};

enum action request = NONE;
int input_stdin = 0;
char* input_file = NULL;
int task = 0;

char *platform_filename = NULL, *parameters_filename = NULL;
enum scheduler scheduler;

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
	char * strend;
	while(*argv != NULL)
	{
//		cerr << "[DEBUG] argv = \"" << *argv << "\"." << endl;
//		cerr << "[DEBUG] strcmp(*argv, \"--tasks\") == \"" << strcmp(*argv, "--tasks") << "\"." << endl;
		if(!strcmp(*argv, "--get") || !strcmp(*argv, "-g"))
		{
			char *opt = *argv;
			argv++;

			if(!strcmp(*argv, "name"))
			{
				set_action(NAME);
			}
			else if(!strcmp(*argv, "tasks"))
			{
				set_action(TASKS);
			}
			else if(!strcmp(*argv, "task"))
			{
				set_action(TASK);

				argv++;
				if(**argv != '-')
				{
					task = strtol(*argv, &strend, 10);

					if((size_t)strend - (size_t)(*argv) < strlen(*argv) || task <= 0)
					{
						cerr << "[WARN ] Invalid Missing task ID provided (\"" << *argv << "\"). Aborting." << endl;
						exit(1);
					}
				}
				else
				{
					cerr << "[WARN ] Missing task name to read information from. Ignoring \"" << opt << "\" directive." << endl;
					argv--;
				}
			}
			else if(!strcmp(*argv, "schedule"))
			{
				set_action(SCHEDULE);
			}
			else
			{
				cerr << "[WARN ] Invalid value for \"get\": \"" << argv << "\". Ignoring \"" << opt << "\" directive." << endl;
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
		else if(!strcmp(*argv, "--parameters") || !strcmp(*argv, "-t"))
		{
			// Read next argument
			argv++;
			if(*argv[0] == '-')
			{	
				cerr << "[WARN ] Cannot read parameters from standard input or invalid option for parameters (\"" << argv << "\" starts with '-'). Ignoring." << endl;
				argv--;
			}
			else
			{
				parameters_filename = *argv;
			}
		}
		else if(!strcmp(*argv, "--taskgraph") || !strcmp(*argv, "-t"))
		{
			// Get to next parameter
			char *opt = *argv;
			argv++;

			if(!strcmp(*argv, "-"))
			{
				input_stdin = 1;
			} else
			{
				if(**argv != '-')
				{
					input_file = *argv;
				}
				else
				{
					cerr << "[WARN ] Invalid value \"" << argv << "\" for option \"" << opt << "\". Ignoring \"" << opt << "\" directive." << endl;
					argv--;
				}
			}
		}
		else if(!strcmp(*argv, "--scheduler") || !strcmp(*argv, "-s"))
		{
			argv++;

			if(!strcmp(*argv, "crown_binary_annealing_allocation_off"))
			{
				scheduler = CROWN_BINARY_ANNEALING_ALLOCATION_OFF;
			}
			else
			{
				cerr << "[WARN ] Invalid field for read action: \"" << argv << "\". Ignoring \"" << *argv << "\" directive." << endl;
				argv--;
			}
		}

		// Read next argument
		argv++;
	}
}

int main(int argc, char **argv)
{
	read_args(argv);
	
	GraphML input;
	Taskgraph *tg;
	if(input_stdin != 0)
	{
		tg = input.parse(cin);
	}
	else
	{
		if(input_file != NULL)
		{
			std::ifstream myfile;
			myfile.open (input_file, std::ios::in);
			tg = input.parse(myfile);
			myfile.close();
		}
		else
		{
			cerr << "[ERROR] No input to read  taskgraph from. Aborting." << endl;
			exit(1);
		}
	}

	switch(request)
	{
		case NAME:
			cout << tg->getName() << endl;	
		break;
		case TASKS:
			for(set<Task>::const_iterator i = tg->getTasks().begin(); i != tg->getTasks().end(); i++)
			{
				cout << i->getId() << " ";
			}
			cout << endl;
		break;
		case TASK:
			cout << tg->findTask(task).getName() << endl;
		break;
		case SCHEDULE:
			switch(scheduler)
			{
				default:
					cerr << "[WARN ] No scheduler specified. Using \"crown_binary_annealing_allocation_off\"." << endl;
				case CROWN_BINARY_ANNEALING_ALLOCATION_OFF:
				{
					Platform *arch;
					Algebra param;

					// Load parameters
					if(parameters_filename != NULL)
					{
						ifstream parameters(parameters_filename, std::ios::in);
						param = AmplInput(AmplInput::floatHandlers()).parse(parameters);
						parameters.close();
					}
					else
					{
						// TODO: create a default parameter set
						/*
						# Frequency cost
						param alpha := 3.0000000;

						# Makespan priority
						param zeta := 0.1000000;

						# Energy saving at idle time
						param epsilon := 0.0;

						# Crown base
						param b := 2;
						*/

						Scalar<float> alpha = Scalar<float>("alpha", 3);
						Scalar<float> zeta = Scalar<float>("zeta", 0.1);
						Scalar<float> epsilon = Scalar<float>("epsilon", 0.0);
						Scalar<float> b = Scalar<float>("b", 2);

						param.insert(&alpha);
						param.insert(&zeta);
						param.insert(&epsilon);
						param.insert(&b);
					}

					// Load platform
					if(platform_filename != NULL)
					{
						ifstream platform(platform_filename, std::ios::in);
						arch = AmplPlatform().parse(platform);
						platform.close();
					}
					else
					{
						cerr << "[ERROR] Missing platform to schedule for. Aborting." << endl;
						exit(1);
					}

					// We already have the taskgraph in tg
					// Run the scheduler
					Algebra schedule = tg->buildAlgebra(arch);
					schedule = schedule.merge(arch->buildAlgebra());
					schedule = schedule.merge(param);
					schedule = crown_binary_annealing_allocation_off(schedule, 8, 0.6, 0.9, 2, 2, 0.5);
					schedule = CrownUtil::crownToSchedule(schedule);
					SnekkjaCSchedule().dump(cout, Schedule("crown_binary_annaling_allocation_", tg, schedule));
				}	
				break;
			}
		break;
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
