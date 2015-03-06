#include "schedeval.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include "target_functions.h"
#include "taskgraph_handling.h"
#include "schedule_handling.h"
#include "code_generation.h"

#ifndef SCHEDEVAL_LOGFILE_DIR
#define SCHEDEVAL_LOGFILE_DIR "/tmp"
#endif
using namespace std;
std::ostream *log_out;  // used across the program instead of cout
int main(int argc, char *argv[])
{

  int args_left = argc -1;
  int i = 1;
  int do_compile = 1;
  int do_run = 1;
  int dump_powerdata = 0;
  int stdlog = 0;
  double walltime = -1;
  char noargs[] = "";
  char *target_args = noargs;
  fstream *logfile;
  log_out = &cout;
  while(args_left > 0 && *argv[i] == '-')
    {
   
      if(strcmp(argv[i],"-run") == 0)
	{ 
	  do_compile=0;
	  do_run = 1;
	}

      else if(strcmp(argv[i],"-skip_target") == 0 ||(strcmp(argv[i],"-compile") == 0 ))
	{
	  do_compile = 1;
	  do_run = 0;
	}

      else if(strcmp(argv[i],"-walltime")== 0)
	{
	  walltime = atof(argv[++i]);
	  --args_left;
	}
      else if(strcmp(argv[i],"-args") == 0)
	{
	  target_args = argv[++i];
	  --args_left;
	}
      else if(strcmp(argv[i],"-dumppower") == 0)
	{
	  dump_powerdata = 1;
	}
      else if(strcmp(argv[i],"-stdlog") == 0)
	{
	  ostringstream path;
	  path << SCHEDEVAL_LOGFILE_DIR <<  "/schedeval_info.log";
	  string s(path.str());
	  logfile = new fstream(path.str().c_str(),ios_base::out);
	  log_out = logfile;
	  stdlog = 1;
	}
      else
	{
	  *log_out << "Flag " << argv[i] << " not implemented. Ignored" << endl;
	}
      --args_left;
      ++i;
    }
  
  if(args_left != 2)
    {
      cerr << "Usage: " << argv[0] << " <taskgraph> <schedule>" << endl;
      return 1;
    }
  
  char *taskgraph_path = argv[i];
  char *schedule_path = argv[i+1];
  taskgraph_t *taskgraph; 
  schedule_t *schedule; 
  char path[128];
  //printf("taskgraph: %s\n",taskgraph_path);
  *log_out << "Taskgraph path: " << taskgraph_path << endl;

  if(taskgraph_load(taskgraph_path, &taskgraph) != 0)
    {
      cerr << "Error loading task graph, aborting.\n";
      return 1;
    }

  taskgraph_printinfo(taskgraph);

  *log_out << "Schedule path: " << schedule_path << ".\n";  
  if(schedule_load(schedule_path, &schedule) != 0)
    {
      cerr << "Error loading schedule, aborting.\n";
      taskgraph_destroy(taskgraph);
      return 1;
    }

  if(do_compile)
    {
      schedule_printinfo(schedule);
      //printf(" Info:Compiling AUT.\n");
      *log_out << "Info: Compiling AUT.\n";
      if(target_compile_aut(taskgraph) != 0)
	{
	  cerr << "Error: Compilation of AUT did not go ok, aborting.\n";
	  taskgraph_destroy(taskgraph);
	  schedule_destroy(schedule);
	  return 1;
	}
      
#define PATH_TO_TASK_DECLARATIONS_C "src/target_exec/task_declarations.c" 
      *log_out << "info: Generating code.\n";
      if(generate_code(schedule,taskgraph,PATH_TO_TASK_DECLARATIONS_C,walltime) != 0)
	{
	  cerr << "Error: Code generataion did not go ok, aborting.\n";
	  taskgraph_destroy(taskgraph);
	  schedule_destroy(schedule);
	  return 1;
	}
      
      *log_out << "info: Compiling final exec.\n";
 
      int retval = target_compile_exec(taskgraph,schedule);
      if(retval != 0)
	{
	  cerr << "Final compilation did not go ok, aborting.\n";
	  taskgraph_destroy(taskgraph);
	  schedule_destroy(schedule);
	  return 1;
	}
      else
	{
	*log_out << "info: Compilation successful.\n";
	}
    }
  

  if(do_run)
    {
      *log_out << "info: Setting up target and running benchmark.\n";
      path_to_exec(taskgraph,schedule,path);
      int retval = target_setup(number_of_nodes(taskgraph));
     if(retval != 0)
	{
	  cerr << "Warning: Setup failed. Trying to run anyway...\n";
	}
      retval = target_run(path,target_args,schedule);
      if(retval != 0)
	{
	  cerr << "Error when trying to run. return value: " << retval << endl;
	}
      else 
	{
	  if(dump_powerdata)
	    {
	      fetch_print_powerdata(path);
	    }
	}
    }
  *log_out << "info: Done with " << taskgraph_getname(taskgraph) << ".\n";
  taskgraph_destroy(taskgraph);
  schedule_destroy(schedule);
  if(stdlog)
    {
      ((fstream*)log_out)->close();
    }
  return 0;
}
