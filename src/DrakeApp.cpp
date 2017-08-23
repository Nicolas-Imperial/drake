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


//#include <libxml++/libxml++.h>
#include <exception>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <string>

#include <pelib/Schedule.hpp>
#include <drake/DrakeApp.hpp>

#include <pelib/Scalar.hpp>
#include <pelib/Vector.hpp>
#include <pelib/Matrix.hpp>
#include <pelib/Set.hpp>
#include <pelib/Task.hpp>

#include <pelib/CastException.hpp>
#include <pelib/ParseException.hpp>

#ifndef debug
#if 10
#define debug(expr) cout << "[" << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << "] " << #expr << " = \"" << expr << "\"." << endl;
#else
#define debug(var)
#endif
#endif

using namespace pelib;
using namespace drake;
using namespace std;

DrakeApp::~DrakeApp()
{
	// Do nothing
}

extern char _binary_drake_app_c_start;
extern size_t _binary_drake_app_c_size;

void
DrakeApp::dump(ostream& os, const Schedule &schedule, const Taskgraph &tg, const Platform &pt) const
{
#if EXPERIMENTAL_CODE
	std::string drake_app_c = std::string(&_binary_drake_app_c_start).substr(0, (size_t)(&_binary_drake_app_c_size));

	os << drake_app_c;
#else
	size_t p = pt.getCores().size();
	size_t n = tg.getTasks().size();
	map<int, set<const Task*> > tasks, producers, consumers;
	os << "#include <stdlib.h> " << endl << endl;
	os << "#include <drake/schedule.h> " << endl << endl;
	os << "#include <drake/platform.h> " << endl << endl;

	/*
	size_t max_rate = 0;
	for(set<Link>::const_iterator i = tg.getLinks().begin(); i != tg.getLinks().end(); i++)
	{
		max_rate = max_rate < i->getRate() ? i->getRate() : max_rate;
	}
	*/

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		const Task &t = *i;
		os <<
			"#define TASK_NAME " << t.getName() << endl <<
			"#define TASK_MODULE " << t.getModule() << endl <<
			"#include <drake/node.h>" << endl <<
			"#define DONE_" << t.getName() << " 1" << endl << endl;
	}

	os << 
		"int drake_task_number()" << endl <<
		"{" << endl <<
		"	return " << n << ";" << endl <<
		"}" << endl << endl <<
		"const char* drake_task_name(size_t index)" << endl <<
		"{" << endl <<
		"	switch(index - 1)" << endl << "	{" << endl;
	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "		case " << std::distance(tg.getTasks().begin(), i) << ":" << endl << "			return (const char*)\"" << i->getName() << "\";" << endl << "		break;" << endl;
	}
	os << "		default:" << endl << "			return \"invalid task id\";" << endl << "		break;" << endl <<
		"	}" << endl << "}" << endl << endl <<
		"void drake_schedule_init(drake_schedule_t* schedule)" << endl <<
		"{" << endl <<
		"	schedule->core_number = " << p << ";" << endl << 
		"	schedule->task_number = " << n << ";" << endl <<
		"	schedule->stage_time = " << tg.getDeadline(pt) << ";" << endl << endl <<
		"	schedule->unique_tasks_in_core = (size_t*)malloc(sizeof(size_t) * schedule->core_number);" << endl <<
		"	schedule->tasks_in_core = (size_t*)malloc(sizeof(size_t) * schedule->core_number);" << endl;

	os << endl << "	schedule->task_name = (const char**)malloc(sizeof(const char*) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	schedule->task_name[" << std::distance(tg.getTasks().begin(), i) << "] = (const char*)\"" << i->getName() << "\";" << endl;
	}
	os << endl;

	os << endl << "	schedule->task_workload = (double*)malloc(sizeof(double) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	schedule->task_workload[" << std::distance(tg.getTasks().begin(), i) << "] = " << i->getWorkload() << ";" << endl;
	}
	os << endl;

	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		os << "	schedule->tasks_in_core[" << i->first - 1 << "] = " << schedule.getTasks(i->first).size() << ";" << endl;
	}
	os << endl;

	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		os << "	schedule->unique_tasks_in_core[" << i->first - 1 << "] = " << schedule.getTasks(i->first).size() << ";" << endl;
	}
	os << endl;

	os << endl << "	schedule->consumers_in_core = (size_t*)malloc(sizeof(size_t) * schedule->core_number);" << endl; 

	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		const Platform::islands i1 = pt.getSharedMemoryIslands(i->first);
		const set<int> islands = pt.getSharedMemoryIslands(i1);
		//os << "	schedule->consumers_in_core[" << i->first - 1 << "] = " << schedule.getRemoteSharedMemoryIslandConsumers(islands, tg, pt).size() << ";" << endl;
		os << "	schedule->consumers_in_core[" << i->first - 1 << "] = " << schedule.getRemoteConsumers(i->first, tg, pt).size() << ";" << endl;
	}

	os << endl << "	schedule->producers_in_core = (size_t*)malloc(sizeof(size_t) * schedule->core_number);" << endl;

	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		const Platform::islands i1 = pt.getSharedMemoryIslands(i->first);
		const set<int> islands = pt.getSharedMemoryIslands(i1);
		//os << "	schedule->producers_in_core[" << i->first - 1 << "] = " << schedule.getRemoteSharedMemoryIslandProducers(islands, tg, pt).size() << ";" << endl;
		os << "	schedule->producers_in_core[" << i->first - 1 << "] = " << schedule.getRemoteProducers(i->first, tg, pt).size() << ";" << endl;
	}

	os << endl << "	schedule->consumers_in_task = (size_t*)malloc(sizeof(size_t) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	schedule->consumers_in_task[" << std::distance(tg.getTasks().begin(), i) << "] = " << i->getConsumers().size() << ";" << endl;
	}

	os << endl << "	schedule->producers_in_task = (size_t*)malloc(sizeof(size_t) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	schedule->producers_in_task[" << std::distance(tg.getTasks().begin(), i) << "] = " << i->getProducers().size() << ";" << endl;
	}

	os << endl << "	schedule->remote_consumers_in_task = (size_t*)malloc(sizeof(size_t) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		//os << "	schedule->remote_consumers_in_task[" << std::distance(tg.getTasks().begin(), i) << "] = " << schedule.getRemoteSharedMemoryIslandTaskConsumers(*i, tg, pt).size() << ";" << endl;
		os << "	schedule->remote_consumers_in_task[" << std::distance(tg.getTasks().begin(), i) << "] = " << schedule.getRemoteTaskConsumers(*i, tg, pt).size() << ";" << endl;
	}

	os << endl << "	schedule->remote_producers_in_task = (size_t*)malloc(sizeof(size_t) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		//os << "	schedule->remote_producers_in_task[" << std::distance(tg.getTasks().begin(), i) << "] = " << schedule.getRemoteSharedMemoryIslandTaskProducers(*i, tg, pt).size() << ";" << endl;
		os << "	schedule->remote_producers_in_task[" << std::distance(tg.getTasks().begin(), i) << "] = " << schedule.getRemoteTaskProducers(*i, tg, pt).size() << ";" << endl;
	}

	os << endl << "	schedule->producers_id = (size_t**)malloc(sizeof(size_t*) * schedule->task_number);" << endl;
	os << "	schedule->producers_rate = (size_t**)malloc(sizeof(size_t*) * schedule->task_number);" << endl;
	os << "	schedule->producers_name = (const char***)malloc(sizeof(const char**) * schedule->task_number);" << endl;
	os << "	schedule->input_name = (const char***)malloc(sizeof(const char**) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		// Producer's id
		os << "	schedule->producers_id[" << std::distance(tg.getTasks().begin(), i) << "] = (size_t*)";
		if (i->getProducers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(size_t) * " << i->getProducers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
			{
				os << "	schedule->producers_id[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = " << std::distance(tg.getTasks().begin(), tg.getTasks().find(*(*j)->getProducer())) + 1 << ";" << endl;
				counter++;
			}
		}

		// Producer's rate
		os << "	schedule->producers_rate[" << std::distance(tg.getTasks().begin(), i) << "] = (size_t*)";
		if (i->getProducers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(size_t) * " << i->getProducers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
			{
				const Link *link = *j;
				os << "	schedule->producers_rate[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = " << (float)link->getProducerRate() << ";" << endl;
				counter++;
			}
		}

		// Producer's name
		os << "	schedule->producers_name[" << std::distance(tg.getTasks().begin(), i) << "] = (const char**)";
		if (i->getProducers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(const char*) * " << i->getProducers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
			{
				const Link *link = *j;
				os << "	schedule->producers_name[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = (const char*)\"" << link->getProducerName() << "\";" << endl;
				counter++;
			}
		}
		os << "	schedule->input_name[" << std::distance(tg.getTasks().begin(), i) << "] = (const char**)";
		if (i->getProducers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(const char*) * " << i->getProducers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
			{
				const Link *link = *j;
				os << "	schedule->input_name[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = (const char*)\"" << link->getConsumerName() << "\";" << endl;
				counter++;
			}
		}
	}

	os << endl << "	schedule->consumers_id = (size_t**)malloc(sizeof(size_t*) * schedule->task_number);" << endl;
	os << "	schedule->consumers_rate = (size_t**)malloc(sizeof(size_t*) * schedule->task_number);" << endl;
	os << "	schedule->consumers_name = (const char***)malloc(sizeof(const char**) * schedule->task_number);" << endl;
	os << "	schedule->output_name = (const char***)malloc(sizeof(const char**) * schedule->task_number);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		// Consumer's ID
		os << "	schedule->consumers_id[" << std::distance(tg.getTasks().begin(), i) << "] = (size_t*)";
		if (i->getConsumers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(size_t) * " << i->getConsumers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
			{
				os << "	schedule->consumers_id[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = " << std::distance(tg.getTasks().begin(), tg.getTasks().find(*(*j)->getConsumer())) + 1 << ";" << endl;
				counter++;
			}
		}

		// Link rate
		os << "	schedule->consumers_rate[" << std::distance(tg.getTasks().begin(), i) << "] = (size_t*)";
		if (i->getConsumers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(size_t) * " << i->getConsumers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
			{
				const Link *link = *j;
				os << "	schedule->consumers_rate[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = " << link->getConsumerRate() << ";" << endl;
				counter++;
			}
		}

		// Consumer's name
		os << "	schedule->consumers_name[" << std::distance(tg.getTasks().begin(), i) << "] = (const char**)";
		if (i->getConsumers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(const char*) * " << i->getConsumers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
			{
				const Link *link = *j;
				os << "	schedule->consumers_name[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = (const char*)\"" << link->getConsumerName() << "\";" << endl;
				counter++;
			}
		}
		os << "	schedule->output_name[" << std::distance(tg.getTasks().begin(), i) << "] = (const char**)";
		if (i->getConsumers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "malloc(sizeof(const char*) * " << i->getConsumers().size() << ");" << endl;
			size_t counter = 0;
			for(set<const Link*>::const_iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
			{
				const Link *link = *j;
				os << "	schedule->output_name[" << std::distance(tg.getTasks().begin(), i) << "][" << counter << "] = (const char*)\"" << link->getProducerName() << "\";" << endl;
				counter++;
			}
		}
	}

	os << endl << "	schedule->schedule = (drake_schedule_task_t**)malloc(sizeof(drake_schedule_task_t*) * schedule->core_number);" << endl;

	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		os << "	schedule->schedule[" << i->first - 1 << "] = (drake_schedule_task_t*)malloc(sizeof(drake_schedule_task_t) * " << i->second.size() << ");" << endl;

		size_t counter = 0;
		for(Schedule::sequence::const_iterator j = i->second.begin(); j != i->second.end(); j++)
		{
			os << "	schedule->schedule[" << i->first - 1 << "][" << counter << "].id = " << std::distance(tg.getTasks().begin(), tg.getTasks().find(*j->second.first)) + 1 << ";" << endl
				<< "	schedule->schedule[" << i->first - 1 << "][" << counter << "].start_time = " << j->second.first->getStartTime() << ";" << endl;

			set<float>::iterator iter = (*pt.getCores().begin())->getFrequencies().find(j->second.first->getFrequency());
			if(iter != (*pt.getCores().begin())->getFrequencies().end())
			{
				os << "	schedule->schedule[" << i->first - 1 << "][" << counter << "].frequency = " << std::distance((*pt.getCores().begin())->getFrequencies().begin(), iter) << ";" << endl;
			}
			else
			{
				throw PelibException("Trying to schedule a task to run on frequency not supported by target platform");
			}
			counter++;
		}
	}

	os << "}" << endl << endl << "void drake_schedule_destroy(drake_schedule_t* schedule)" << endl << "{" << endl;

	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		os << "	free(schedule->schedule[" << i->first - 1 << "]);" << endl;
		/*
		for(size_t j = 0; j < i->second.size(); j++)
		{
			os << "	free(schedule->schedule[" << i->first - 1 << "][" << i->second.size() - j - 1 << "]);" << endl;
		}
		*/
	}

	os << endl << "	free(schedule->schedule);" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	free(schedule->consumers_id[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		os << "	if(schedule->consumers_rate[" << std::distance(tg.getTasks().begin(), i) << "] != NULL)" << endl;
		os << "	{" << endl;
		os << "		free(schedule->consumers_rate[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl;
		os << "	}" << endl;
		os << "	free(schedule->consumers_name[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		os << "	free(schedule->output_name[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		/*
		for(size_t j = 0; j < i->getConsumers().size(); j++)
		{
			os << "	free(schedule->consumers_id[" << std::distance(tg.getTasks().begin(), i) << "][" << i->getConsumers().size() - j - 1 << "]);" << endl;
		}
		*/
	}

	os << "	free(schedule->consumers_id);" << endl
		<< "	free(schedule->consumers_rate);" << endl
		<< "	free(schedule->consumers_name);" << endl
		<< endl;
	
	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	free(schedule->producers_id[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		os << "	if(schedule->producers_rate[" << std::distance(tg.getTasks().begin(), i) << "] != NULL)" << endl;
		os << "	{" << endl;
		os << "		free(schedule->producers_rate[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		os << "	}" << endl;
		os << "	free(schedule->producers_name[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		os << "	free(schedule->input_name[" << std::distance(tg.getTasks().begin(), i) << "]);" << endl; 
		/*
		for(size_t j = 0; j < i->getProducers().size(); j++)
		{
			os << "	free(schedule->producers_id[" << std::distance(tg.getTasks().begin(), i) << "][" << i->getProducers().size() - j - 1 << "]);" << endl;
		}
		*/
	}

	os << "	free(schedule->producers_id);" << endl
		<< "	free(schedule->producers_rate);" << endl
		<< "	free(schedule->producers_name);" << endl
		<< "	free(schedule->task_workload);" << endl
		<< "	free(schedule->remote_producers_in_task);" << endl
		<< "	free(schedule->remote_consumers_in_task);" << endl
		<< "	free(schedule->producers_in_task);" << endl
		<< "	free(schedule->consumers_in_task);" << endl
		<< "	free(schedule->producers_in_core);" << endl
		<< "	free(schedule->consumers_in_core);" << endl
		<< "	free(schedule->unique_tasks_in_core);" << endl
		<< "	free(schedule->tasks_in_core);" << endl
		<< "	free(schedule->task_name);" << endl
		<< "}" << endl
		<< endl
		<< "size_t" << endl
		<< "drake_task_width(task_tp task)"
		<< "{" << endl
		<< "	size_t task_width[" << tg.getTasks().size() << "] = {";

		for(set<Task>::const_iterator i = schedule.getUniqueTasks().begin(); i != schedule.getUniqueTasks().end(); i++)
		{
			os << schedule.getCores(*i).size() << ", ";
		}
	
	os << "};" << endl
		<< "	return task_width[task->id - 1];" << endl
		<< "}" << endl
		<< endl
		<< "size_t" << endl
		<< "drake_core_id(task_tp task)"  
		<< "{" << endl
		<< "	size_t local_core_id[" << pt.getCores().size() << "][" << tg.getTasks().size() << "] = {" << endl;

	// Initializes a counter for cores allocated to tasks
	size_t *core_counter = (size_t*)malloc(sizeof(size_t) * tg.getTasks().size());
	for(size_t i = 0; i < tg.getTasks().size(); i++)
	{
		core_counter[i] = 0;
	}

	// Generates initialized core
	for(Schedule::table::const_iterator i = schedule.getSchedule().begin(); i != schedule.getSchedule().end(); i++)
	{
		os << "			{";
		for(set<Task>::const_iterator j = tg.getTasks().begin(); j != tg.getTasks().end(); j++)
		{
			Schedule::sequence::const_iterator k;
			for(k = i->second.begin(); k != i->second.end(); k++)
			{
				if(k->second.first->getName().compare(j->getName()) == 0)
				{
					size_t task_id = std::distance(tg.getTasks().begin(), (set<Task>::iterator)j);
					os << core_counter[task_id] << ", ";
					core_counter[task_id]++;
					break;
				}
			}

			if(k == i->second.end())
			{
				os << pt.getCores().size() << ", ";
			}
		}
		os << "}," << endl;
	}
	free(core_counter);
	os << "		};" << endl
		<< "	return local_core_id[drake_platform_core_id()][task->id - 1];" << endl
		<< "}" << endl
		<< endl
		<< "void*" << endl
		<< "drake_function(size_t id, task_status_t status)" << endl
		<< "{" << endl
		<< "	switch(id)" << endl
		<< "	{" << endl
		<< "		default:" << endl
		<< "			// TODO: Raise an alert" << endl
		<< "		break;" << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "		case " << std::distance(tg.getTasks().begin(), tg.getTasks().find(*i)) + 1<< ":" << endl
			<< "			switch(status)" << endl
			<< "			{" << endl
			<< "				case TASK_INVALID:" << endl
			<< "					return 0;" << endl
			<< "				break;" << endl
			<< "				case TASK_INIT:" << endl
			<< "					return (void*)&drake_init(" << i->getModule() << ", " << i->getName() << ");" << endl
			<< " 				break;" << endl
			<< "  				case TASK_START:" << endl
			<< "  					return (void*)&drake_start(" << i->getModule() << ", " << i->getName() << ");" << endl
			<< "  				break;" << endl
			<< "  				case TASK_RUN:" << endl
			<< "  					return (void*)&drake_run(" << i->getModule() << ", " << i->getName() << ");" << endl
			<< "  				break;" << endl
			<< "  				case TASK_KILLED:" << endl
			<< "  					return (void*)&drake_kill(" << i->getModule() << ", " << i->getName() << ");" << endl
			<< "  				break;" << endl
			<< "  				case TASK_ZOMBIE:" << endl
			<< "  					return 0;" << endl
			<< "  				break;" << endl
			<< "  				case TASK_DESTROY:" << endl
			<< "  					return (void*)&drake_destroy(" << i->getModule() << ", " << i->getName() << ");" << endl
			<< "  				break;" << endl
			<< "  				default:" << endl
			<< "  					return 0;" << endl
			<< "  				break;" << endl
			<< "  			}" << endl
			<< "  		break;" << endl;
	}

	os << endl
		<< "	}" << endl
		<< endl
		<< "	return 0;" << endl
		<< "}" << endl;
#endif
}

void
DrakeApp::dump(ostream& os, const Schedule *schedule, const Taskgraph *tg, const Platform *pt) const
{
	this->dump(os, *schedule, *tg, *pt);
}

DrakeApp*
DrakeApp::clone() const
{
	return new DrakeApp();
}
