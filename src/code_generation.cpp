#include "code_generation.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
//#include <memory.h>
#include <iostream>
extern "C"{
#include <libxml/parser.h>
}

using namespace std;

//utility function
static int get_int_attr(xmlNode *node,const char* attrname)
{
 xmlChar *attrname_xml = xmlCharStrdup(attrname);
  xmlChar* ch = xmlGetProp(node,attrname_xml);
  if(ch == NULL)
    {
      cerr << "Error: attribute """ << attrname << """ does not exist in this file.\n";
      cerr.flush();
      // Following this we will purposly dereference and crash.
    }
  int retval =  atoi((char*)ch);
  free(ch);
  free(attrname_xml);
  return retval;
}
 //utility function
static double get_double_attr(xmlNode *node,const char* attrname)
{
  xmlChar *attrname_xml = xmlCharStrdup(attrname);
  xmlChar* ch = xmlGetProp(node,attrname_xml);
  if(ch == NULL)
    {
      cerr << "Error: attribute """ << attrname << """ does not exist in this file.\n";
      cerr.flush();
      // Following this we will purposly dereference and crash.
    }
  double retval =  atof((char*)ch);
  free(ch);
  free(attrname_xml);
  return retval;
}
  //utility function
static char* get_str_attr(xmlNode *node,const char* attrname)
{
  xmlChar *attrname_xml = xmlCharStrdup(attrname);
  xmlChar* ch = xmlGetProp(node,attrname_xml);
  if(ch == NULL)
    {
      cerr << "Error: attribute """ << attrname << """ does not exist in this file.\n";
      cerr.flush();
      // Following this we will purposly dereference and crash.
    }
  free(attrname_xml);
  return (char*) ch;
}

//return number of characters written
static int make_dummies(char* dest, int times)
{
  if(times < 1) return 0;

  int i = 0;
  for(i = 0; i < (times-1) *5 ; i+=5)
    {
      //strcpy(dest+i, "dummy,");
      strcpy(dest+i,"NULL,");
    }
  //strcpy(dest+i, "dummy");
  strcpy(dest+i,"NULL");
  return 5*times-1;
}

static int generate_edges_array(taskgraph_t *taskgraph, schedule_t *schedule, FILE *file)
{
  if(file == NULL)
    {
      return 1;
    }
  //    igraph_vector_t gtypes, vtypes, etypes;
  //igraph_strvector_t gnames, vnames, enames;
  long int id;


  fprintf(file,"#define CHANNEL_CONNECTIONS {");
  for (id=0; id<igraph_ecount(taskgraph); id++) {

    //printf("Edge %li (%i-%i): ", id, (int)IGRAPH_FROM(taskgraph,id), (int)IGRAPH_TO(taskgraph,id));
    int from_nodeid = IGRAPH_FROM(taskgraph,id);
    int to_nodeid = IGRAPH_TO(taskgraph,id);

    const char *from_taskid = VAS(taskgraph,"taskid",from_nodeid);
    const char *to_taskid = VAS(taskgraph,"taskid",to_nodeid);
    if(from_taskid == NULL || to_taskid == NULL)
      {
	cerr <<"Error: The taskgraph is malformed. Some tasknodes ("
	     << from_nodeid << ", " << to_nodeid 
	     << ") are not defined.\n";
	return 1;
      }

    int from_core;
    int from_ordering;
    if(schedule_find_task(schedule,from_taskid,&from_core,&from_ordering) != 0)
      {
	cerr << "Error: could not find task " << from_taskid << " in schedule.\n";
	return 1;
      }
    int to_core;
    int to_ordering;
    if(schedule_find_task(schedule,to_taskid,&to_core,&to_ordering) != 0)
      {
	cerr << "Error: could not find task " << to_taskid << "in schedule.\n";
	return 1;
      }

    //printf("channel: from %i,%i to %i,%i\n",from_core,from_ordering,to_core,to_ordering);
    if(id > 0) 
      {
	  fprintf(file,",");
      }

    // Allow ordering to start at any number. Internal numbering starts with 1
    int offset = 1- schedule_find_min_ordering(schedule);
    fprintf(file,"{{%i,%i},{%i,%i}}",from_core,from_ordering+offset,to_core,to_ordering+offset);
  }
  fprintf(file,"}\n");
  fprintf(file,"#define TOTAL_NO_CHANNELS %i\n", igraph_ecount(taskgraph));


  return 0;
}
int generate_code(schedule_t *schedule,taskgraph_t *taskgraph, const char *file_path,double walltime)
{

  xmlNode  *root = xmlDocGetRootElement(schedule);
  const char *autname = taskgraph_getname(taskgraph);//get_str_attr(root,"autname");
  char *schedulename = schedule_getname(schedule);//get_str_attr(root,"name");

  const int BIGENOUGH = 100000;
  int len = 0;
  char* schedule_array_code= (char*) malloc(BIGENOUGH*sizeof(char));
  char* schedule_header_code=(char*) malloc(BIGENOUGH*sizeof(char));
  len += sprintf(schedule_array_code,
		 "// THIS IS A GENERATED FILE. DO NOT MODIFY MANUALLY.\n"
		 "#define NO_CORES %i\n" 
		 //	 "#include \"task_declarations-%s.h\" \n\n"
		 
		 //		 "static int dummy(void){  printf (\"DUMMY:%%s\\n\", __FUNCTION__); return 0;}\n"

		 "#define TOTAL_NO_TASKS %i\n",
		 schedule_get_num_cores(schedule),//get_int_attr(root,"cores"),
		 schedule_get_num_tasks(schedule));// get_int_attr(root,"tasks"));
  if(walltime > 0)
    {
      len += sprintf(schedule_array_code+len,"#define WALLTIME %f\n",walltime);
    }
  len += sprintf(schedule_array_code+len,"#define ROUNDTIME %f\n",get_double_attr(root,"roundtime"));
  int hlen = sprintf(schedule_header_code,"// THIS IS A GENERATED FILE. DO NOT MODIFY MANUALLY.\n"
		     "#ifndef  TASK_DECLARATIONS_H\n"
		     "#define TASK_DECLARATIONS_H\n\n"
		     "typedef int (*fptr_t)(void);\n"
		     "typedef int (*setup_fptr_t)(channelc_t, channelc_t, int, char **);\n"
		     "typedef void (*destroy_fptr_t)(void);\n");

  char setup[BIGENOUGH/4];
  char prerun[BIGENOUGH/4];
  char run[BIGENOUGH/4];
  char destroy[BIGENOUGH/4];
  int total_no_tasks[48] = {0};
  //int *frequency_all = (int*) malloc(number_of_nodes(taskgraph)*sizeof(int));
  //char ** taskid_all = (char**) malloc(number_of_nodes(taskgraph)* sizeof(char*));
  int no_of_tasks = get_int_attr(root,"tasks");
  int *frequency_all = (int*) malloc(no_of_tasks*sizeof(int));
  char ** taskid_all = (char**) malloc(no_of_tasks* sizeof(char*));

  int setup_len = 0, prerun_len = 0, run_len = 0, destroy_len = 0;
  int counted_no_of_cores = 0;

  xmlNode *cur_node = NULL;
  setup[setup_len++] = '{';
  prerun[prerun_len++] = '{';
  run[run_len++] = '{';
  destroy[destroy_len++] = '{'; 
  int taskindex = 0;
  int max_frequency_all[48] = {0};
  for (cur_node = root->children; cur_node; cur_node = cur_node->next)  //all cores
    {
      if (cur_node->type == XML_ELEMENT_NODE)
	{
	  int current_core = get_int_attr(cur_node,"coreid");
	  ++counted_no_of_cores;
	  xmlNode  *cur_task = NULL;

	  setup[setup_len++] = '{';
	  prerun[prerun_len++] = '{';
	  run[run_len++] = '{';
	  destroy[destroy_len++] = '{';
	  int counted_no_tasks = 0;
	  for (cur_task = cur_node->children; cur_task; cur_task = cur_task->next)  //all tasks for core
	    {
	      if (cur_task->type == XML_ELEMENT_NODE)
		{
		  char *taskid = get_str_attr(cur_task,"taskid");
		  taskid_all[taskindex] = taskid;
		  frequency_all[taskindex] = get_int_attr(cur_task,"frequency");
		  max_frequency_all[current_core] = max_frequency_all[current_core] < frequency_all[taskindex] ? frequency_all[taskindex] : max_frequency_all[current_core];
		  const char *taskname = taskgraph_find_taskname(taskgraph,taskid);
		  
		 
		  // function pointer array
		  ++counted_no_tasks;
		  setup_len += sprintf(setup+setup_len,"%s_%s_%s_setup,",autname,taskname,taskid);
		  prerun_len += sprintf(prerun+prerun_len,"%s_%s_%s_prerun,",autname,taskname,taskid);
		  run_len += sprintf(run+run_len,"%s_%s_%s_run,",autname,taskname,taskid);
		  destroy_len += sprintf(destroy+destroy_len,"%s_%s_%s_destroy,",autname,taskname,taskid);
		  //printf("node type: %i, name: %s, %s\n",cur_task->type, cur_task->name, schedulename);

		  // function declarations
		  hlen += sprintf(schedule_header_code+hlen,"int %s_%s_%s_setup(channelc_t incoming_channels, channelc_t outgoing_channels,int argc, char *argv[]);\n",autname,taskname,taskid);
		  hlen += sprintf(schedule_header_code+hlen,"int %s_%s_%s_prerun(void);\n",autname,taskname,taskid);
		  hlen += sprintf(schedule_header_code+hlen,"int %s_%s_%s_run(void);\n",autname,taskname,taskid);
		  hlen += sprintf(schedule_header_code+hlen,"void %s_%s_%s_destroy(void);\n",autname,taskname,taskid);
		  hlen += sprintf(schedule_header_code+hlen,"\n");
		  //free(taskid); //no, this is done at the end of this function
		  ++taskindex;
		}
	    }
	  total_no_tasks[current_core] = counted_no_tasks;

	  setup_len += make_dummies(setup+setup_len,no_of_tasks-counted_no_tasks);
	  strcpy(setup+ setup_len,"},"); 
	  setup_len +=2;

	  prerun_len += make_dummies(prerun+prerun_len,no_of_tasks-counted_no_tasks);
	  strcpy(prerun+ prerun_len,"},"); 
	  prerun_len +=2;

	  run_len += make_dummies(run+run_len,no_of_tasks-counted_no_tasks);
	  strcpy(run+ run_len,"},"); 
	  run_len +=2;

	  destroy_len += make_dummies(destroy+destroy_len,no_of_tasks-counted_no_tasks);
	  strcpy(destroy+destroy_len,"},"); 
	  destroy_len +=2;

	}

    }
  strcpy(setup + setup_len -1,"}"); 
  strcpy(prerun + prerun_len -1,"}"); 
  strcpy(run + run_len -1,"}"); 
  strcpy(destroy + destroy_len -1,"}"); 

  hlen += sprintf(schedule_header_code+hlen,"#endif\n");
  len += sprintf(schedule_array_code + len,
	     "#define TASKS_SETUP_ALL %s\n"
	     "#define TASKS_PRERUN_ALL %s\n"
	     "#define TASKS_RUN_ALL %s\n"
	     "#define TASKS_DESTROY_ALL %s\n\n",
	     setup,prerun,run,destroy);
  int i;


  len += sprintf(schedule_array_code + len,"#define FREQUENCY_ALL {");
  for (i = 0; i < number_of_nodes(taskgraph); i++)
    {
      len += sprintf(schedule_array_code + len,"%i%c",frequency_all[i],',');
    }
  len += sprintf(schedule_array_code + len-1,"%s","}\n")-1;
 
  len += sprintf(schedule_array_code + len,"%s","#define MAX_FREQUENCY_ALL {");
  for (i = 0; i < get_int_attr(root,"cores"); i++)
    {
      len += sprintf(schedule_array_code + len,"%i%c",	max_frequency_all[i],',');
    }
  len += sprintf(schedule_array_code + len-1,"%s","}\n")-1;


  len += sprintf(schedule_array_code + len,"%s","#define NO_TASKS_ALL {");
  for (i = 0; i < get_int_attr(root,"cores"); i++)
    {
      len += sprintf(schedule_array_code + len,"%i%c",total_no_tasks[i],',');
    }
  len += sprintf(schedule_array_code + len-1,"%s","}\n") -1;


  len += sprintf(schedule_array_code + len,"%s","#define TASKID_ALL {");
  for (i = 0; i < number_of_nodes(taskgraph); i++)
    {
      len += sprintf(schedule_array_code + len,"\"%s\",",taskid_all[i]);
    }
  len += sprintf(schedule_array_code + len-1,"%s","}\n");


  FILE *f =fopen (file_path, "w");
  //printf("%s\n",schedule_header_code);

  if(f == NULL)
    {
      cerr << "Error, could not open " << file_path << ".\n";
      return 1;
    }
  fprintf (f, "%s",schedule_array_code);
  int retval = generate_edges_array(taskgraph, schedule, f);
  fclose (f);  
  free(schedule_array_code);
  
  if(retval != 0)
    {
      return retval;
    }
  char path[100];
  sprintf(path,"include/target_exec/task_declarations.h");

  f =fopen (path, "w");
  if(f == NULL)
    {
      cerr << "Error, could not open " << path << ".\n";
      return 1;
    }
  fprintf (f, "%s",schedule_header_code);  
  fclose (f);
  free(schedule_header_code);
  //free(autname);
  free(schedulename);
  //for(i = 0; i < number_of_nodes(taskgraph); i++)
  for(i = 0; i < no_of_tasks; i++)
    {
      free(taskid_all[i]);
    }
  free(taskid_all);
  free(frequency_all); 
  return 0;
}
