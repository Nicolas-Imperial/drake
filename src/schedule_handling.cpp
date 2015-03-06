#include "schedule_handling.h"
#include "schedeval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <limits.h>
using namespace std;
static int inited = 0;
void schedule_init()
{
  xmlInitParser();
  inited = 1;
}


static int get_int_attr(xmlNode *node,const char* attrname)
{
 xmlChar *attrname_xml = xmlCharStrdup(attrname);
  xmlChar* ch = xmlGetProp(node,attrname_xml);
  if(ch == NULL)
    {
      cerr << "Error, attribute \"" << attrname << "\" not found in schedule." << endl;
      free(ch);
      free(attrname_xml);
      exit(1);
    }

      
  int retval =  atoi((char*)ch);
  free(ch);
  free(attrname_xml);
  return retval;
}

static bool attr_exists(xmlNode *node,const char* attrname) 
{
  xmlChar *attrname_xml = xmlCharStrdup(attrname);                                                                                                                                                                     xmlChar* ch = xmlGetProp(node,attrname_xml);                                                                                                                                                                   
  bool exists = ch != NULL;
  if(ch != NULL)
    {
      free(ch);     
    }                                                                                                                                                                                              
  free(attrname_xml);                                                                                                                                                                                               
  return exists;
}

static double get_float_attr(xmlNode *node,const char* attrname)
{
 xmlChar *attrname_xml = xmlCharStrdup(attrname);
  xmlChar* ch = xmlGetProp(node,attrname_xml);
  if(ch == NULL)
    {
      cerr << "Error, attribute \"" << attrname << "\" not found in schedule." << endl;
      free(ch);
      free(attrname_xml);
      exit(1);
    }

      
  double retval =  atof((char*)ch);
  free(ch);
  free(attrname_xml);
  return retval;
}
static char* get_str_attr(xmlNode *node,const char* attrname)
{
  xmlChar *attrname_xml = xmlCharStrdup(attrname);
  xmlChar* ch = xmlGetProp(node,attrname_xml);
 if(ch == NULL)
    {
      cerr << "Error, attribute \"" << attrname << "\" not found in schedule." << endl;
      free(attrname_xml);
      exit(1);
    }
  free(attrname_xml);
  return (char*) ch;
}

char* schedule_getname(schedule_t *schedule)
{
  
  xmlNode        *root;
  root = xmlDocGetRootElement(schedule);
  return get_str_attr(root,"name");
}

int schedule_get_num_cores(schedule_t *schedule)
{
  xmlNode *root =  xmlDocGetRootElement(schedule);
  return get_int_attr(root,"cores");

}
int schedule_get_num_tasks(schedule_t *schedule)
{
  xmlNode *root =  xmlDocGetRootElement(schedule);
  return get_int_attr(root,"tasks");
}


// Return where the start time would be ordered within all tasks scheduled on specified core
static int startingtime_to_ordering(xmlNode* core_node, double start_time)
{
  xmlNode  *cur_task = NULL;              
  int ordering = 0;
  for (cur_task = core_node->children; cur_task; cur_task = cur_task->next)  //all tasks for core 
    {
      if (cur_task->type == XML_ELEMENT_NODE)                                                                                                                                                               
	{
	  double t = get_float_attr(cur_task,"starting_time");
	  if(t < start_time)
	    {
	      ordering += 1;
	    }
	}
    }
  return ordering;
}

int schedule_find_task(schedule_t *schedule,const char *taskid, int *core, int *ordering)
{ 
  xmlNode  *root = xmlDocGetRootElement(schedule);
  xmlNode *cur_node = NULL;
  for (cur_node = root->children; cur_node; cur_node = cur_node->next)  //all cores
    {
      if (cur_node->type == XML_ELEMENT_NODE)
	{
	  int current_core = get_int_attr(cur_node,"coreid");
	  
	  xmlNode  *cur_task = NULL;
	  for (cur_task = cur_node->children; cur_task; cur_task = cur_task->next)  //all tasks for core
	    {
	      if (cur_task->type == XML_ELEMENT_NODE)
		{
		  char *current_taskid = get_str_attr(cur_task,"taskid");
		  if(strcmp(taskid,current_taskid) == 0)
		    {
		      if(attr_exists(cur_task,"ordering"))
			{
			  *ordering = get_int_attr(cur_task,"ordering");
			}
		      else
			{
			  double starting_time = get_float_attr(cur_task,"starting_time");
			  *ordering = startingtime_to_ordering(cur_node,starting_time);
			}
		      
		      *core = current_core;
		      //printf("found %s at %i,%i\n",current_taskid,*core,*ordering);
		      free(current_taskid);
		      return 0;
		    }
		  free(current_taskid);
		}
	    }
	}
    }
   return 1; //not found
}


// This is for schedules with the "ordering" attribute instead of start time.
// Ordering can start at 0 or 1 (or some other integer).
// Return what ordering the schedule start with.
int schedule_find_min_ordering(schedule_t *schedule)
{
  xmlNode  *root = xmlDocGetRootElement(schedule);
  xmlNode *cur_node = NULL;
  int min_ordering = INT_MAX;
  for (cur_node = root->children; cur_node; cur_node = cur_node->next)  //all cores
    {
      if (cur_node->type == XML_ELEMENT_NODE)
	{
	  xmlNode  *cur_task = NULL;
	  for (cur_task = cur_node->children; cur_task; cur_task = cur_task->next)  //all tasks for core
	    {
	      if (cur_task->type == XML_ELEMENT_NODE)
		{
		  // This is the common case 
		  if(attr_exists(cur_task,"starting_time"))
		    {
		      return 0;
		    }
		  int ordering = get_int_attr(cur_task,"ordering");
		  min_ordering = ordering < min_ordering ? ordering : min_ordering;
		}
	    }
	}
    }
  return min_ordering;
}


void schedule_printinfo(schedule_t *schedule)
{
  char *name = schedule_getname(schedule);
  *log_out << "Schedule name: " << name << ". Number of cores " << schedule_get_num_cores(schedule) << ".\n";
  free(name);

}
int schedule_load(const char *path, schedule_t **schedule)
{
  if(inited == 0)
    {
      schedule_init();
    }
  xmlDoc *s = xmlReadFile(path,NULL,0);
  if(s == 0)
    {
      cerr << "Error: Could not find or parse schedule at " << path << ".\n";
      return 1;
    }
  *schedule = s;
   return 0;
}



void schedule_destroy(schedule_t *schedule)
{
  xmlFreeDoc(schedule);
  xmlCleanupParser();
  inited = 0;
}
