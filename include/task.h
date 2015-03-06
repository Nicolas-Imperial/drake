#ifndef TASK_H
#define TASK_H

enum task_status {TASK_INVALID,TASK_NEW, TASK_INIT,TASK_START_PRERUN,TASK_PRERUN,TASK_START_RUN, TASK_RUN, TASK_KILLED, TASK_ZOMBIE};

//Declare for the benefit of communication.h
enum task_status;
struct task;
typedef struct task task_t;
typedef task_t* task_tp;
typedef enum task_status task_status_t;
#include "communication.h"

struct task
{
  //int id;
  const char* taskid;
  channelc_t incoming_channels; // Note that these may be local, or remote
  channelc_t outgoing_channels;
  int (*setup)(channelc_t incoming_channels, channelc_t outgoing_channels, int, char **);
  int (*prerun)(void);
  int (*run)(void);
  void (*destroy)(void);
  int frequency;
  task_status_t status;
  /*
    unsigned long long int start_time, stop_time, start_presort, stop_presort, check_time, push_time, work_time, check_errors, check_recv, check_putback, check_feedback, put_reset, put_pop, put_send, check_wait, push_wait, work_wait, work_read, work_write;
    unsigned long long int step_init, step_start, step_check, step_work, step_push, step_killed, step_zombie, step_transition;*/
#ifdef DETAILED_TIMINGS
  unsigned long long int start_prerun, stop_prerun, start_run, stop_run;
#endif
};

#endif
