#include "taskgraph_handling.h"
#include <stdio.h>
#include <string.h>
#include "schedeval.h"
#include <iostream>
int inited = 0;

static void init()
{
  igraph_i_set_attribute_table(&igraph_cattribute_table); //do this to enable attribute fetching
  inited = 1;
}

int taskgraph_load(const char *path, taskgraph_t **taskgraph)
{
  if(!inited)
    {
      init();
    }
  FILE *file = fopen(path,"r");
  if(!file) return IGRAPH_FAILURE;
  igraph_t *loaded_graph = (igraph_t*) malloc(sizeof(igraph_t));
  if(igraph_read_graph_graphml(loaded_graph,file,0) != IGRAPH_SUCCESS)
    {
      free(loaded_graph);
      return IGRAPH_FAILURE;
    }
  fclose(file);
  *taskgraph =  loaded_graph;

  return 0;
}

void taskgraph_destroy(taskgraph_t *taskgraph)
{
  igraph_destroy(taskgraph);
  free(taskgraph);
  inited = 0;
}

const char* taskgraph_find_taskname(const taskgraph_t *taskgraph, const char *taskid)
{
  int nodeid;
    for (nodeid=0; nodeid<igraph_vcount(taskgraph); nodeid++) {
      if (strcmp(VAS(taskgraph,"taskid",nodeid),taskid) == 0)
	{
	  return VAS(taskgraph,"taskname",nodeid);
	}
    }
    return "NONAME";
}

const char* taskgraph_get_taskname(const taskgraph_t *taskgraph, size_t nodeid)
{
  const char* taskname = VAS(taskgraph,"taskname",nodeid);

  //igraph can't read default values, so this is a workaround
  return strcmp(taskname, "") == 0 ? "UNNAMED_TASKNAME": taskname; 
}

const char* taskgraph_get_taskid(const taskgraph_t *taskgraph,size_t nodeid)
{
  const char* taskid = VAS(taskgraph,"taskid",nodeid);
  return strcmp(taskid,"") == 0 ? "UNNAMED_TASKID": taskid;
}

size_t taskgraph_get_numberoftasks(const taskgraph_t *taskgraph)
{
  return igraph_vcount(taskgraph);
}

const char* taskgraph_getname(const taskgraph_t *taskgraph){
  return igraph_cattribute_GAS(taskgraph,"autname");
}

int number_of_nodes(const taskgraph_t *taskgraph)
{
 return igraph_vcount(taskgraph);
}

void taskgraph_printinfo(const taskgraph_t *taskgraph)
{
  int numnodes = number_of_nodes(taskgraph);
  const char* name = taskgraph_getname(taskgraph);
  
  //printf("Application name: %s, %i tasks.\n",name,numnodes);
  *log_out << "Application name: " << name << ", " << numnodes << " tasks.\n";
}
