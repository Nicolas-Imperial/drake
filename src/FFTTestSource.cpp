#define DATATYPE float
#include "snekkja.h"
#include <stdlib.h>
#include <stdio.h>

static int n;
static channel_t out;
int TASKSETUP(channelc_t incoming,channelc_t outgoing, int argc,char *argv[])
{
  out = get_channel(outgoing,0);
  return 0;
}

int TASKPRERUN()
{
  char myid[] = xquotify(TASKID);
  myid[3] = '\0';
  n = atoi(myid);
  printf("My n is %i. capacity is %i",n,capacity(float)(out));
  return 0;
}
static int numel = 0;
int TASKRUN()
{
  if(capacity(float)(out) < 2*n)
    {
      return 1;
    }
  numel++;
  int i,j;
  float val_i,val_r;
  val_r = 0;
  val_i = 0;
  put(float)(out,&val_r);
  put(float)(out,&val_i);
  val_r = 1;
  put(float)(out,&val_r);
  put(float)(out,&val_i);
  val_r = 0;

  for(j = 0; j < n-2; j++)
    {
      put(float)(out,&val_r);
      put(float)(out,&val_i);
    }
  return 1;
}


void TASKDESTROY(){
}
