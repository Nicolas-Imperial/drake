#define DATATYPE float
#include "snekkja.h"

static int n;
static channel_t in, left_out,right_out;
enum turn_t{left, right};
static enum turn_t turn ;
int TASKSETUP(channelc_t incoming,channelc_t outgoing, int argc,char *argv[])
{
  in = get_channel(incoming,0);
  left_out = get_channel(outgoing,0);
  right_out = get_channel(outgoing,1);

  // This is a workaround since schedeval does not support proper 
  // parameterized instantiation yet.
  char myid[] = xquotify(TASKID);
  myid[3] = '\0';
  n = atoi(myid);
  //printf("(%s) My n is %i. capacity is %i",xquotify(TASKID),n,capacity(float)(out));
  turn = left;
  return 0;
}

int TASKPRERUN(){ return 0;}
#define WORKUNIT 128
int TASKRUN()
{
  channel_t out = turn == left ? left_out : right_out;


  if(length(float)(in) < WORKUNIT || capacity(float)(out) < WORKUNIT)
    {
      return 1;
    }

 
  float val,j;
  for(j = 0; j < WORKUNIT; j++)
    {
      get(float)(in,&val);
      put(float)(out,&val);
    }
  turn = turn == left ? left : right;
  return 1;
}


void TASKDESTROY(){
}
