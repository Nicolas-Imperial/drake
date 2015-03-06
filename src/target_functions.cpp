#include "target_functions.h"
#include "schedeval.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

static int run(const char* command, ostream &redirect)
{
   FILE *f = popen(command,"r");
  
  char c;
  for(;;)
    {
      c = fgetc(f);
      if(c== EOF) break;
      redirect << c;
    }

  int retval = pclose(f);
  return retval;
}

int target_setup(int /*number_of_nodes*/)
{
  //int retval = run("sccBmc -c \"i2c off\"",*log_out);
  // when sccBmc is not in PATH
  int retval = run("/opt/sccKit/current/bin/sccBmc -c \"i2c off\"",*log_out);
  return retval;
}

int target_run(const char *path_to_exec, const char* args, schedule_t* schedule)
{
  #define RCCE_PATH "/home/johja659/rcce/trunk/rcce/rccerun"
  char cmd[4096];
  int retval;
  sprintf(cmd,"cp %s /shared/$USER",path_to_exec);
  //retval = system(cmd);
  retval = run(cmd,*log_out);
  if (retval != 0)
    {
      return retval;
    }
  
  sprintf(cmd,"cd /shared/$USER && %s -nue %i -f rccerun_corespec/%s -clock 0.8 `basename %s` %s",RCCE_PATH,schedule_get_num_cores(schedule),"rccerun_00-47",path_to_exec,args);
  *log_out << "\nCommand: " << cmd << "\n" ;
  //retval = system(cmd);
  retval = run(cmd,*log_out);
  if (retval != 0)
    {
      return retval;
    }

  return 0;
}

#ifndef TARGET_PLATFORM 
#define TARGET_PLATFORM "scc_linux"
#endif

void path_to_exec(taskgraph_t *taskgraph,schedule_t *schedule,char *path)
{
  char *schedname = schedule_getname(schedule);
  sprintf(path,"bin/target_exec-%s-%s-%s",TARGET_PLATFORM,taskgraph_getname(taskgraph),schedname);
  free(schedname);
}

int target_compile_exec(taskgraph_t *taskgraph,schedule_t *schedule)
{
  char command[4096];
  char *schedname = schedule_getname(schedule);
  const char *autname = taskgraph_getname(taskgraph);
  sprintf(command,"make -C src/target_exec target_exec TARGET_PLATFORM=%s AUT=%s SCHEDULE=%s 2>&1",
	  TARGET_PLATFORM,autname,schedname);
  //printf("%s\n",command);
  
  //int retval = system(command);
  int retval = run(command,*log_out);
  if(retval !=0)
    {
      cout << "NOPE";
      free(schedname);
      return retval;
    }
  //sprintf(path,"Compilation of target_exec succeeded. Path: bin/target_exec-%s-%s-%s",TARGET_PLATFORM,taskgraph_getname(taskgraph),schedname);
  free(schedname);
  return  0;
}
  
int target_compile_aut(taskgraph_t *taskgraph)
{
    const char *autname = taskgraph_getname(taskgraph);
    char cmd[256];
    size_t id;
    const size_t number_of_tasks = taskgraph_get_numberoftasks(taskgraph);

    int ans = 0;
    for(id = 0; id < number_of_tasks && ans == 0; ++id)
      {
	sprintf(cmd,"make -C src/target_exec aut "
		"TARGET_PLATFORM=scc_linux "
		"AUT=%s "
		"TASKNAME=%s "
		"TASKID=%s 2>&1",
		autname,taskgraph_get_taskname(taskgraph,id),taskgraph_get_taskid(taskgraph,id));
	//ans = system(cmd);
	ans = run(cmd,*log_out);
	//ans = run(cmd,cout);
      }
    if(ans == 0)
      {
	sprintf(cmd,"make -C src/target_exec aut-finalize AUT=%s TARGET_PLATFORM=scc_linux 2>&1",autname);
	ans = run(cmd,*log_out);
	//ans = system(cmd);
      }
    return ans;
}

void fetch_print_powerdata(const char * /*path_to_exec*/)
{
  char cmd[] = "find /shared/$USER/logfiles -type f | xargs ls -1S | head -n 1";
  stringstream ans;
  string largest_logfile_path;
  run(cmd,ans);
  ans >> largest_logfile_path;
  //ifstream data("/shared/johja659/logfiles/measurement-target_exec-core_0-.log");
  ifstream data(largest_logfile_path.c_str());
  if(!data)
    {
      std::cerr << "Error: no logfile present\n";
      return;
    }
  float time,core,mmc;
  string s;
  getline(data,s); // skip first comment line
  vector<float> timedata;
  vector<float> coredata;
  vector<float> mmcdata;
  while(data)
    {
      data >> time >> core >> mmc;
      timedata.push_back(time);
      coredata.push_back(core);
      mmcdata.push_back(mmc);
    }
  int datapoints = timedata.size();
  cout << "#core + mmc power consumption[W]" << endl
       << "time__power [*,*] " << endl
       << ":\t0\t1 :=" << endl;
  for(int i = 0; i < datapoints; i++)
    {
      cout << i << "\t" << timedata[i] << "\t" << coredata[i]+mmcdata[i] << endl;
    }
  cout << ";" << endl << endl;
  cout << "#core power consumption[W]" << endl
       << "time__chippower [*,*] " << endl
       << ":\t0\t1 :=" << endl;
  for(int i = 0; i < datapoints; i++)
    {
      cout << i << "\t" << timedata[i] << "\t" << coredata[i] << endl;
    }
  cout << ";" << endl << endl;
  
   cout << "#mmc power consumption[W]" << endl
	<< "time__mmcpower [*,*] " << endl
	<< ":\t0\t1 :=" << endl;
   for(int i = 0; i < datapoints; i++)
     {
       cout << i << "\t" << timedata[i] << "\t" << mmcdata[i] << endl;
     }
   cout << ";" << endl << endl;
}
