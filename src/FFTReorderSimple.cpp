#define DATATYPE float
#include "snekkja.h"
#include <stdlib.h>
#include <stdio.h>

static int totalData;
static int n;
static channel_t in, out;
int TASKSETUP(channelc_t incoming, channelc_t outgoing, int argc, char *argv[])
{
  in = get_channel(incoming,0);
  out = get_channel(outgoing,0);
  return 0;
}

int TASKPRERUN()
{
  // This is a workaround since schedeval does not support proper 
  // parameterized instantiation yet.
  char myid[] = xquotify(TASKID);
  myid[3] = '\0';
  n = atoi(myid);
  totalData = 2*n;
  //printf("totalData = %i\n",totalData);
  return 0;
}

#define WORKUNIT 128
static int sum = 0;
static int runtimes = 0;
int TASKRUN()
	  {
  if(length(float)(in) < WORKUNIT || capacity(float)(out) < WORKUNIT)
    {
      return 1;
    }
	  
	  

  int i,j;
  float val;
  for(j = 0; j < ((float)WORKUNIT)/(float)totalData; j++)
    {
      //runtimes++;
      for (i = 0; i < totalData; i+=4)
	{
	  peek(float)(in,i,&val);
	  put(float)(out,&val);
	  peek(float)(in,i+1,&val);
	  put(float)(out,&val);	       
	}

      for (i = 2; i < totalData; i+=4)
	{
	  peek(float)(in,i,&val);
	  put(float)(out,&val);
	  peek(float)(in,i+1,&val);
	  put(float)(out,&val);
	}
      for(i = 0; i < totalData; i++)
	{
	  get(float)(in,&val); //discard
	  //if(n==64) printf("got: (%f)\n",val);

	}
    }
  return 1;
}


void TASKDESTROY(){
  //printf("Reorder %s, I ran %i proper times %i values in buffer. capacity, %i. loops %i\n",xquotify(TASKID),runtimes, length(float)(in),capacity(float)(out),WORKUNIT/totalData);
}
