#define DATATYPE float
#include "snekkja.h"
#include <stdlib.h>
#include <stdio.h>

static channel_t in;
int TASKSETUP(channelc_t incoming,channelc_t outgoing, int argc,char *argv[])
{
  in = get_channel(incoming,0);
  return 0;
}

int TASKPRERUN()
{
  return 0;
}

static int numel = 0;
int TASKRUN()
{
  int num = length(float)(in);
  while(num >= 2)
    {
      float val_r, val_i;
      get(float)(in,&val_r);
      get(float)(in,&val_i);
      numel +=1;
      //printf("\t %i: (%f,%f)\n",numel,val_r,val_i);
      num -= 2;
    }
  return 1;
}


void TASKDESTROY(){
  printf("Numel produced: %i\n",numel);
}
