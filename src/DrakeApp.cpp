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
#include <algorithm>
#include <string>

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

string
uppercase(string str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::toupper);
	return str;
}

void
DrakeApp::dump(ostream& os, const Schedule &schedule, const Taskgraph &tg, const Platform &pt) const
{
	// Get a list of links per core
	map<unsigned int, set<const Link*> > core_link, core_input_distributed_link, core_output_distributed_link;
	for(set<Link>::iterator i = schedule.getLinks().begin(); i != schedule.getLinks().end(); i++)
	{
		if(core_link.find(i->getQueueBuffer().getCore()) == core_link.end())
		{
			core_link.insert(pair<unsigned int, set<const Link*> >());
		}
		if(core_input_distributed_link.find(i->getQueueBuffer().getCore()) == core_input_distributed_link.end())
		{
			core_input_distributed_link.insert(pair<unsigned int, set<const Link*> >());
		}
		if(core_output_distributed_link.find(i->getQueueBuffer().getCore()) == core_output_distributed_link.end())
		{
			core_output_distributed_link.insert(pair<unsigned int, set<const Link*> >());
		}

		core_link[i->getQueueBuffer().getCore()].insert(&*i);
	}

	map<Task, set<const Link*> > task_input_distributed, task_output_distributed;
	for(set<Link>::iterator i = schedule.getLinks().begin(); i != schedule.getLinks().end(); i++)
	{
		if(task_input_distributed.find(*i->getConsumer()) == task_input_distributed.end())
		{
			task_input_distributed.insert(pair<Task, set<const Link*> >(*i->getConsumer(), set<const Link*>()));
		}
		if(task_input_distributed.find(*i->getProducer()) == task_output_distributed.end())
		{
			task_output_distributed.insert(pair<Task, set<const Link*> >(*i->getProducer(), set<const Link*>()));
		}

		if((Buffer::MemoryType)((int)i->getQueueBuffer().getMemoryType() & (int)Buffer::memoryAccessMask) == Buffer::MemoryType::distributed)
		{
			task_input_distributed[*i->getConsumer()].insert(&*i);
			task_output_distributed[*i->getProducer()].insert(&*i);
			core_input_distributed_link[i->getConsumerBuffer().getCore()].insert(&*i);
			core_output_distributed_link[i->getProducerBuffer().getCore()].insert(&*i);
		}
	}

	os << "#include <iostream>" << endl;
	os << endl;
	os << "#include <drake.h>" << endl;
	os << "#include <pelib/char.h>" << endl;
	os << "#include <drake/platform.h>" << endl;
	os << endl;
	os << "#ifndef debug" << endl;
	os << "#if 10" << endl;
	os << "#define debug(expr) std::cout << \"[\" << __FILE__ << \":\" << __FUNCTION__ << \":\" << __LINE__ << \"] \" << #expr << \" = \\\"\" << expr << \"\\\".\" << std::endl;" << endl;
	os << "#else" << endl;
	os << "#define debug(var)" << endl;
	os << "#endif" << endl;
	os << "#endif" << endl;
	os << endl;

	bool at_least_one_header = false;
	for(set<Link>::iterator i = schedule.getLinks().begin(); i != schedule.getLinks().end(); i++)
	{
		if(i->getQueueBuffer().getHeader().compare(string()) != 0)
		{
			os << "#include <" << i->getQueueBuffer().getHeader() << ">" << endl;
			at_least_one_header = true;
		}
	}

	if(at_least_one_header)
	{
		os << endl;
		os << endl;
	}

	for(set<Task>::iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "#define " << uppercase(i->getName()) << " " << std::distance(tg.getTasks().begin(), i) << endl;
	}
	os << endl;

	for(set<Task>::iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "int drake_init(" << i->getModule() << ", " << i->getName() << ")(void*);" << endl;
		os << "int drake_start(" << i->getModule() << ", " << i->getName() << ")();" << endl;
		os << "int drake_run(" << i->getModule() << ", " << i->getName() << ")();" << endl;
		os << "int drake_kill(" << i->getModule() << ", " << i->getName() << ")();" << endl;
		os << "int drake_destroy(" << i->getModule() << ", " << i->getName() << ")();" << endl;
		os << endl;
	}

	os << "drake_application_t*" << endl;
	os << "drake_application_build()" << endl;
	os << "{" << endl;
	os << "	drake_memory_t type = (drake_memory_t)(DRAKE_MEMORY_PRIVATE | DRAKE_MEMORY_SMALL_CHEAP);" << endl;
	os << "	unsigned int level = 0;" << endl;
	os << endl;
	os << "	drake_application_t *app = drake_platform_get_application_details();" << endl;
	os << "	app->number_of_cores = " << pt.getCores().size() << ";" << endl;
	os << "	app->core_private_link = (drake_core_private_link_t*)drake_platform_malloc(sizeof(drake_core_private_link_t) * app->number_of_cores, drake_platform_core_id(), type, level);" << endl;
	for(map<unsigned int, set<const Link*> >::const_iterator i = core_link.begin(); i != core_link.end(); i++)
	{
		os << "	app->core_private_link[" << i->first << "].number_of_links = " << i->second.size() << ";" << endl;
		os << "	app->core_private_link[" << i->first << "].link = (drake_abstract_link_t*)drake_platform_malloc(sizeof(drake_abstract_link_t) * app->core_private_link[0].number_of_links, drake_platform_core_id(), type, level);" << endl;
	}
	os << endl;
	os << "	app->number_of_tasks = " << tg.getTasks().size() << ";" << endl;
	os << "	app->task = (drake_task_t*)drake_platform_malloc(sizeof(drake_task_t) * app->number_of_tasks, drake_platform_core_id(), type, level);" << endl;
	for(set<Task>::iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	app->task[" << uppercase(i->getName()) << "].init = drake_init(" << i->getModule() << ", " << i->getName() << ");" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].start = drake_start(" << i->getModule() << ", " << i->getName() << ");" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].run = drake_run(" << i->getModule() << ", " << i->getName() << ");" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].kill = drake_kill(" << i->getModule() << ", " << i->getName() << ");" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].destroy = drake_destroy(" << i->getModule() << ", " << i->getName() << ");" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].asleep = 0;" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].state = DRAKE_TASK_STATE_INIT;" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].name = \"" << i->getName().c_str() << "\";" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].instance = NULL;" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].number_of_input_links = " << i->getProducers().size() << ";" << endl;
		os << "	app->task[" << uppercase(i->getName()) << "].number_of_output_links = " << i->getConsumers().size() << ";" << endl;
		os << endl;
	}

	for(map<unsigned int, set<const Link*> >::const_iterator i = core_link.begin(); i != core_link.end(); i++)
	{
		const set<const Link*> &list = i->second;
		unsigned int core = i->first;
		for(set<const Link*>::const_iterator j = list.begin(); j != list.end(); j++)
		{
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].producer = &app->task[" << uppercase((*j)->getProducer()->getName()) << "];" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].consumer = &app->task[" << uppercase((*j)->getConsumer()->getName()) << "];" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue = (cfifo_t(char)*)drake_platform_malloc(sizeof(cfifo_t(char)), drake_platform_core_id(), type, level);" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].token_size = sizeof(" << (*j)->getQueueBuffer().getType() << ");" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->capacity = " << (*j)->getQueueBuffer().getSize() << " * app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].token_size;" << endl;
			//os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->buffer = NULL;" << endl;
			//os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->buffer = (char*)drake_platform_malloc(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->capacity, drake_platform_core_id(), (drake_memory_t)(" << Buffer::memoryTypeToString((*j)->getQueueBuffer().getMemoryType()) << "), " << (*j)->getQueueBuffer().getLevel() << ");" << endl;
			//os << "	std::cout << \"(\\\"" << (*j)->getProducer()->getName() << "\\\" -> \\\"" << (*j)->getConsumer()->getName() << "\\\")(" << (*j)->getProducerName() << " -> " << (*j)->getConsumerName() << "): [\" << (size_t)(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->buffer) << \", \" << app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->capacity << \"]\" << std::endl;" << endl;
			//os << "	pelib_init(cfifo_t(char))(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue);" << endl;
			os << "	std::cout << \"(\\\"" << (*j)->getProducer()->getName() << "\\\" -> \\\"" << (*j)->getConsumer()->getName() << "\\\")(" << (*j)->getProducerName() << " -> " << (*j)->getConsumerName() << "):\" << pelib_cfifo_length(char)(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue) << std::endl;" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].type = (drake_memory_t)(" << Buffer::memoryTypeToString((*j)->getQueueBuffer().getMemoryType()) << ");" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].core = " << core << ";" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].producer_name = (char*)\"" << (*j)->getProducerName() << "\";" << endl;
			os << "	app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].consumer_name = (char*)\"" << (*j)->getConsumerName() << "\";" << endl;
		}
		os << endl;
	}

	for(set<Task>::iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "	app->task[" << uppercase(i->getName()) << "].input_link = ";
		if(i->getProducers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "(drake_abstract_link_t**)drake_platform_malloc(sizeof(drake_abstract_link_t*) * app->task[" << uppercase(i->getName()) << "].number_of_input_links, drake_platform_core_id(), type, level);" << endl;
			unsigned int core, link;
			for(set<const Link*>::const_iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
			{
				for(map<unsigned int, set<const Link*> >::const_iterator k = core_link.begin(); k != core_link.end(); k++)
				{
					for(set<const Link*>::const_iterator l = k->second.begin(); l != k->second.end(); l++)
					{
						if(**j == **l)
						{
							link = std::distance(k->second.begin(), l);
							core = k->first;
						}
					}
				}
				os << "	app->task[" << uppercase(i->getName()) << "].input_link[" << std::distance(i->getProducers().begin(), j) << "] = &app->core_private_link[" << core << "].link[" << link << "];" << endl;
			}
		}

		os << "	app->task[" << uppercase(i->getName()) << "].output_link = ";
		if(i->getConsumers().size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "(drake_abstract_link_t**)drake_platform_malloc(sizeof(drake_abstract_link_t*) * app->task[" << uppercase(i->getName()) << "].number_of_output_links, drake_platform_core_id(), type, level);" << endl;
			unsigned int core, link;
			for(set<const Link*>::const_iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
			{
				bool found = false;
				for(map<unsigned int, set<const Link*> >::const_iterator k = core_link.begin(); k != core_link.end(); k++)
				{
					for(set<const Link*>::const_iterator l = k->second.begin(); l != k->second.end(); l++)
					{
						if(**j == **l)
						{
							link = std::distance(k->second.begin(), l);
							core = k->first;
							found = true;
							break;
						}
					}
					if(found)
					{
						break;
					}
				}
				if(found)
				{
					os << "	app->task[" << uppercase(i->getName()) << "].output_link[" << std::distance(i->getConsumers().begin(), j) << "] = &app->core_private_link[" << core << "].link[" << link << "];" << endl;
				}
				else
				{
					debug("Link in taskgraph was not found in schedule.");
					debug(i->getName());
					debug((*j)->getConsumer()->getName());
					debug((*j)->getProducerName());
					debug((*j)->getConsumerName());
					abort();
				}
			}
		}

		os << "	app->task[" << uppercase(i->getName()) << "].input_distributed_link = ";
		if(task_input_distributed.find(*i) != task_input_distributed.end() || task_input_distributed[*i].size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "(drake_input_distributed_link_t*)drake_platform_malloc(sizeof(drake_input_distributed_link_t*) * app->task[" << uppercase(i->getName()) << "].number_of_input_distributed_links, drake_platform_core_id(), type, level);" << endl;
			unsigned int core, link;
			for(set<const Link*>::const_iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
			{
				bool found = false;
				for(map<unsigned int, set<const Link*> >::const_iterator k = core_link.begin(); k != core_link.end(); k++)
				{
					for(set<const Link*>::const_iterator l = k->second.begin(); l != k->second.end(); l++)
					{
						if(**j == **l && (Buffer::MemoryType)((int)(*j)->getQueueBuffer().getMemoryType() & (int)Buffer::memoryAccessMask) == Buffer::MemoryType::distributed)
						{
							link = std::distance(k->second.begin(), l);
							core = k->first;
							found = true;
							break;
						}
					}
					if(found)
					{
						break;
					}
				}
				if(found)
				{
					os << "	app->task[" << uppercase(i->getName()) << "].input_distributed_link[" << std::distance(i->getConsumers().begin(), j) << "] = &app->core_input_distributed_link[" << core << "].link[" << link << "];" << endl;
				}
				else
				{
					debug("Link in taskgraph was not found in schedule.");
					abort();
				}
			}
		}

		os << "	app->task[" << uppercase(i->getName()) << "].output_distributed_link = ";
		if(task_output_distributed.find(*i) != task_output_distributed.end() || task_output_distributed[*i].size() == 0)
		{
			os << "NULL;" << endl;
		}
		else
		{
			os << "(drake_output_distributed_link_t*)drake_platform_malloc(sizeof(drake_output_distributed_link_t*) * app->task[" << uppercase(i->getName()) << "].number_of_output_distributed_links, drake_platform_core_id(), type, level);" << endl;
			unsigned int core, link;
			for(set<const Link*>::const_iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
			{
				bool found = false;
				for(map<unsigned int, set<const Link*> >::const_iterator k = core_link.begin(); k != core_link.end(); k++)
				{
					for(set<const Link*>::const_iterator l = k->second.begin(); l != k->second.end(); l++)
					{
						if(**j == **l && (Buffer::MemoryType)((int)(*j)->getQueueBuffer().getMemoryType() & (int)Buffer::memoryAccessMask) == Buffer::MemoryType::distributed)
						{
							link = std::distance(k->second.begin(), l);
							core = k->first;
							found = true;
							break;
						}
					}
					if(found)
					{
						break;
					}
				}
				if(found)
				{
					os << "	app->task[" << uppercase(i->getName()) << "].output_distributed_link[" << std::distance(i->getProducers().begin(), j) << "] = &app->core_output_distributed_link[" << core << "].link[" << link << "];" << endl;
				}
				else
				{
					debug("Link in taskgraph was not found in schedule.");
					abort();
				}
			}
		}
		os << endl;
	}

	os << "	app->core_input_distributed_link = (drake_core_input_distributed_link_t*)drake_platform_malloc(sizeof(drake_core_input_distributed_link_t) * app->number_of_cores, drake_platform_core_id(), type, level);" << endl;
	for(unsigned int i = 0; i < pt.getCores().size(); i++)
	{
		if(core_input_distributed_link.find(i) == core_input_distributed_link.end() || core_input_distributed_link[i].size() == 0)
		{
			os << "	app->core_input_distributed_link[" << i << "].number_of_links = " << 0 << ";" << endl;
		}
		else
		{
			os << "	app->core_input_distributed_link[" << i << "].number_of_links = " << core_input_distributed_link[i].size() << ";" << endl;
			os << "	app->core_input_distributed_link[" << i << "].link = (drake_input_distributed_link_t*)drake_platform_malloc(sizeof(drake_input_distributed_link_t) * " << core_input_distributed_link[i].size() << ", drake_platform_core_id(), type, level); " << endl;
			for(set<const Link*>::const_iterator j = core_input_distributed_link[i].begin(); j != core_input_distributed_link[i].end(); j++)
			{
				unsigned int link;
				bool found = false;
				for(set<const Link*>::const_iterator l = core_link[i].begin(); l != core_link[i].end(); l++)
				{
					if(**j == **l && (Buffer::MemoryType)((int)(*j)->getQueueBuffer().getMemoryType() & (int)Buffer::memoryAccessMask) == Buffer::MemoryType::distributed)
					{
						link = std::distance(core_link[i].begin(), l);
						found = true;
						break;
					}
				}
				if(found)
				{
					os << "	app->core_input_distributed_link[" << i << "].link[" << std::distance(core_input_distributed_link[i].begin(), j) << "].link = &app->core_private_link[" << i << "].link[" << link << "];" << endl;
					os << "	app->core_input_distributed_link[" << i << "].link[" << std::distance(core_input_distributed_link[i].begin(), j) << "].read = (size_t*)drake_platform_malloc(sizeof(size_t), " << (*j)->getConsumerBuffer().getCore() << ", (drake_memory_t)(" << Buffer::memoryTypeToString((*j)->getConsumerBuffer().getMemoryType()) << "), " << (*j)->getConsumerBuffer().getLevel() << ");" << endl;
					os << "	app->core_input_distributed_link[" << i << "].link[" << std::distance(core_input_distributed_link[i].begin(), j) << "].state = (drake_task_state_t*)drake_platform_malloc(sizeof(drake_task_state_t), " << (*j)->getConsumerBuffer().getCore() << ", (drake_memory_t)(" << Buffer::memoryTypeToString((*j)->getConsumerBuffer().getMemoryType()) << "), " << (*j)->getConsumerBuffer().getLevel() << ");" << endl;
				}
				else
				{
					debug("Link in taskgraph was not found in schedule.");
					abort();
				}
			}
		}
	}
	os << endl;

	os << "	app->core_output_distributed_link = (drake_core_output_distributed_link_t*)drake_platform_malloc(sizeof(drake_core_output_distributed_link_t) * app->number_of_cores, drake_platform_core_id(), type, level);" << endl;
	for(unsigned int i = 0; i < pt.getCores().size(); i++)
	{
		if(core_output_distributed_link.find(i) == core_output_distributed_link.end() || core_output_distributed_link[i].size() == 0)
		{
			os << "	app->core_output_distributed_link[" << i << "].number_of_links = " << 0 << ";" << endl;
		}
		else
		{
			os << "	app->core_output_distributed_link[" << i << "].number_of_links = " << core_output_distributed_link[i].size() << ";" << endl;
			os << "	app->core_output_distributed_link[" << i << "].link = (drake_output_distributed_link_t*)drake_platform_malloc(sizeof(drake_output_distributed_link_t) * " << core_output_distributed_link[i].size() << ", drake_platform_core_id(), type, level); " << endl;
			for(set<const Link*>::const_iterator j = core_output_distributed_link[i].begin(); j != core_output_distributed_link[i].end(); j++)
			{
				unsigned int link;
				bool found = false;
				for(set<const Link*>::const_iterator l = core_link[i].begin(); l != core_link[i].end(); l++)
				{
					if(**j == **l && (Buffer::MemoryType)((int)(*j)->getQueueBuffer().getMemoryType() & (int)Buffer::memoryAccessMask) == Buffer::MemoryType::distributed)
					{
						link = std::distance(core_link[i].begin(), l);
						found = true;
						break;
					}
				}
				if(found)
				{
					os << "	app->core_output_distributed_link[" << i << "].link[" << std::distance(core_output_distributed_link[i].begin(), j) << "].link = &app->core_private_link[" << i << "].link[" << link << "];" << endl;
					os << "	app->core_output_distributed_link[" << i << "].link[" << std::distance(core_output_distributed_link[i].begin(), j) << "].write = (size_t*)drake_platform_malloc(sizeof(size_t), " << (*j)->getProducerBuffer().getCore() << ", (drake_memory_t)(" << Buffer::memoryTypeToString((*j)->getProducerBuffer().getMemoryType()) << "), " << (*j)->getProducerBuffer().getLevel() << ");" << endl;
				}
				else
				{
					debug("Link in taskgraph was not found in schedule.");
					abort();
				}
			}
		}
	}

	os << "	app->number_of_task_instances = (unsigned int*)drake_platform_malloc(sizeof(unsigned int) * app->number_of_cores, drake_platform_core_id(), type, level);" << endl;
	for(unsigned int i = 0; i < pt.getCores().size(); i++)
	{
		os << "	app->number_of_task_instances[" << i << "] = " << schedule.getSchedule().find(i)->second.size() << ";" << endl;
	}
	os << "	app->schedule = (drake_exec_task_t**)drake_platform_malloc(sizeof(drake_exec_task_t*) * app->number_of_cores, drake_platform_core_id(), type, level);" << endl;
	for(unsigned int i = 0; i < pt.getCores().size(); i++)
	{
		os << "	app->schedule[" << i << "] = (drake_exec_task_t*)drake_platform_malloc(sizeof(drake_exec_task_t) * " << schedule.getSchedule().find(i)->second.size() << ", drake_platform_core_id(), type, level);" << endl;
		for(Schedule::sequence::const_iterator j = schedule.getSchedule().find(i)->second.begin(); j != schedule.getSchedule().find(i)->second.end(); j++)
		{
			const Task *task = j->second.first;
			size_t core_index = std::distance(schedule.getSchedule().find(i)->second.begin(), j);
			size_t task_index = std::distance(tg.getTasks().begin(), tg.getTasks().find(*task));
			
			//os << "	app->schedule[" << i << "][" << core_index << "].task = &app->task[" << task_index << "];" << endl;
			os << "	app->schedule[" << i << "][" << core_index << "].task = &app->task[" << uppercase(task->getName()) << "];" << endl;
			os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].start_time = " << j->first << ";" << endl;
			os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].width = " << j->second.first->getWidth() << ";" << endl;
			os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].frequency = " << std::distance(pt.getCore(i)->getFrequencies().begin(), pt.getCore(i)->getFrequencies().find(j->second.first->getFrequency() / pt.getCore(i)->getFrequencyUnit())) + 1 << ";" << endl;
			if(std::next(j, 1) == schedule.getSchedule().find(i)->second.end())
			{
				os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].next = NULL;" << endl;
				os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].round_next = NULL;" << endl;
			}
			else
			{
				os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].next = &app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) + 1 << "];" << endl;
				os << "	app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) << "].round_next = &app->schedule[" << i << "][" << std::distance(schedule.getSchedule().find(i)->second.begin(), j) + 1 << "];" << endl;
			}
		}
	}

	for(map<unsigned int, set<const Link*> >::const_iterator i = core_link.begin(); i != core_link.end(); i++)
	{
		const set<const Link*> &list = i->second;
		unsigned int core = i->first;
		for(set<const Link*>::const_iterator j = list.begin(); j != list.end(); j++)
		{
			//os << "	std::cout << \"(\\\"" << (*j)->getProducer()->getName() << "\\\" -> \\\"" << (*j)->getConsumer()->getName() << "\\\")(" << (*j)->getProducerName() << " -> " << (*j)->getConsumerName() << "): [\" << (size_t)(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->buffer) << \", \" << app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->capacity << \"]\" << std::endl;" << endl;
			//os << "	std::cout << \"(\\\"" << (*j)->getProducer()->getName() << "\\\" -> \\\"" << (*j)->getConsumer()->getName() << "\\\")(" << (*j)->getProducerName() << " -> " << (*j)->getConsumerName() << "):\" << pelib_cfifo_length(char)(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue) << std::endl;" << endl;
			//os << "	debug((void*)(app->core_private_link[" << core << "].link[" << std::distance(list.begin(), j) << "].queue->buffer));" << endl;
		}
		os << endl;
	}
	os << endl;
	os << "	return app;" << endl;
	os << "}" << endl;

	os << "unsigned int" << endl;
	os << "drake_application_number_of_tasks()" << endl;
	os << "{" << endl;
	os << "	return drake_platform_get_application_details()->number_of_tasks;" << endl;
	os << "}" << endl;
	os << endl;

	for(set<Task>::const_iterator i = tg.getTasks().begin(); i != tg.getTasks().end(); i++)
	{
		os << "drake_task_tp" << endl;
		os << "drake_task(" << i->getModule() << ", " << i->getName() << ")()" << endl;
		os << "{" << endl;
		os << "	return &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
		os << "}" << endl;
		os << endl;

		for(set<const Link*>::iterator j = i->getProducers().begin(); j != i->getProducers().end(); j++)
		{
			set<Link>::iterator link_iter = schedule.getLinks().find(**j);
			if(link_iter == schedule.getLinks().cend())
			{
				debug("Cannot find link in schedule");
				abort();
			}
			const Link &link = *schedule.getLinks().find(**j);
			os << "size_t" << endl;
			os << "drake_input_capacity(" << i->getModule() << ", " << i->getName() << ", " << link.getConsumerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->input_link[" << std::distance(i->getProducers().begin(), j) << "];" << endl;
			os << "	size_t ret = drake_buffer_capacity(link);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << "size_t" << endl;
			os << "drake_input_available(" << i->getModule() << ", " << i->getName() << ", " << link.getConsumerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->input_link[" << std::distance(i->getProducers().begin(), j) << "];" << endl;
			os << "	size_t ret = drake_buffer_available(link);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << "size_t" << endl;
			os << "drake_input_available_continuous(" << i->getModule() << ", " << i->getName() << ", " << link.getConsumerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->input_link[" << std::distance(i->getProducers().begin(), j) << "];" << endl;
			os << "	size_t ret = drake_buffer_available_continuous(link);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << link.getQueueBuffer().getType() << "*" << endl;
			os << "drake_input_buffer(" << i->getModule() << ", " << i->getName() << ", " << link.getConsumerName() << ")(size_t skip, size_t *size, " << link.getQueueBuffer().getType() << " **extra)" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->input_link[" << std::distance(i->getProducers().begin(), j) << "];" << endl;
			os << "	" << link.getQueueBuffer().getType() << " *ret = (" << link.getQueueBuffer().getType() << "*)drake_buffer_availableaddr(link, skip, size, (void**)extra);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << "void" << endl;
			os << "drake_input_discard(" << i->getModule() << ", " << i->getName() << ", " << link.getConsumerName() << ")(size_t size)" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->input_link[" << std::distance(i->getProducers().begin(), j) << "];" << endl;
			os << "	drake_buffer_discard(link, size);" << endl;
			os << "}" << endl;
			os << endl;

			os << "void" << endl;
			os << "drake_input_depleted(" << i->getModule() << ", " << i->getName() << ", " << link.getConsumerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->input_link[" << std::distance(i->getProducers().begin(), j) << "];" << endl;
			os << "	drake_buffer_depleted(link);" << endl;
			os << "}" << endl;
			os << endl;
		}

		for(set<const Link*>::iterator j = i->getConsumers().begin(); j != i->getConsumers().end(); j++)
		{
			set<Link>::iterator link_iter = schedule.getLinks().find(**j);
			if(link_iter == schedule.getLinks().cend())
			{
				debug("Cannot find link in schedule");
				abort();
			}
			const Link &link = *schedule.getLinks().find(**j);
			os << "size_t" << endl;
			os << "drake_output_capacity(" << i->getModule() << ", " << i->getName() << ", " << link.getProducerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->output_link[" << std::distance(i->getConsumers().begin(), j) << "];" << endl;
			os << "	size_t ret = drake_buffer_capacity(link);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << "size_t" << endl;
			os << "drake_output_available(" << i->getModule() << ", " << i->getName() << ", " << link.getProducerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->output_link[" << std::distance(i->getConsumers().begin(), j) << "];" << endl;
			os << "	size_t ret = drake_buffer_free(link);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << "size_t" << endl;
			os << "drake_output_available_continuous(" << i->getModule() << ", " << i->getName() << ", " << link.getProducerName() << ")()" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->output_link[" << std::distance(i->getConsumers().begin(), j) << "];" << endl;
			os << "	size_t ret = drake_buffer_free_continuous(link);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << link.getQueueBuffer().getType() << "*" << endl;
			os << "drake_output_buffer(" << i->getModule() << ", " << i->getName() << ", " << link.getProducerName() << ")(size_t *size, " << link.getQueueBuffer().getType() << " **extra)" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->output_link[" << std::distance(i->getConsumers().begin(), j) << "];" << endl;
			os << "	" << link.getQueueBuffer().getType() << " *ret = (" << link.getQueueBuffer().getType() << "*)drake_buffer_freeaddr(link, size, (void**)extra);" << endl;
			os << "	return ret;" << endl;
			os << "}" << endl;
			os << endl;

			os << "void" << endl;
			os << "drake_output_commit(" << i->getModule() << ", " << i->getName() << ", " << link.getProducerName() << ")(size_t size)" << endl;
			os << "{" << endl;
			os << "	drake_task_t *task = &drake_platform_get_application_details()->task[" << std::distance(tg.getTasks().begin(), i) << "];" << endl;
			os << "	drake_abstract_link_t *link = task->output_link[" << std::distance(i->getConsumers().begin(), j) << "];" << endl;
			os << "	drake_buffer_commit(link, size);" << endl;
			os << "}" << endl;
			os << endl;
		}
	}

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
