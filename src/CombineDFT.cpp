#define DATATYPE float
#include "snekkja.h"
#include <math.h>

static int totalData;
static int n;
static channel_t in, out;
static float *w;
int TASKSETUP(channelc_t incoming,channelc_t outgoing, int argc,char *argv[])
{
  in = get_channel(incoming,0);
  out = get_channel(outgoing,0);

  // This is a workaround since schedeval does not support proper 
  // parameterized instantiation yet.
  char myid[] = xquotify(TASKID);
  myid[3] = '\0';
  n = atoi(myid);
  totalData= 2*n;
  w = (float*) malloc(n*sizeof(float));
  return 0;
}

int TASKPRERUN()
{

  float wn_r = (float)cos(2.0 * 3.141592654 / (double)n);
  float wn_i = (float)sin(-2.0 * 3.141592654 / (double)n);
  float real = 1;
  float imag = 0;
  float next_real, next_imag;
  int i;
  for (i=0; i<n; i+=2) {
    w[i] = real;
    w[i+1] = imag;
    next_real = real * wn_r - imag * wn_i;
    next_imag = real * wn_i + imag * wn_r;
    real = next_real;
    imag = next_imag;
  }
    return 0;
}

#define WORKUNIT 128
static int runtimes = 0;
int TASKRUN()
{
  if(length(float)(in) < WORKUNIT || capacity(float)(out) < WORKUNIT)
    {
      return 1;
    }
  //runtimes++;
  float val,j;
  for(j = 0; j < WORKUNIT/totalData; j++)
    {
      int i;
      float results[2*n];
      
      for (i = 0; i < n; i += 2)
	{
	  float y0_r,y0_i,y1_r,y1_i;
	  
	  peek(float)(in,i,  &y0_r);
	  peek(float)(in,i+1,&y0_i);
	  
	  peek(float)(in,n+i,  &y1_r);
	  peek(float)(in,n+i+1,&y1_i);
	  
	  float weight_real = w[i];
	  float weight_imag = w[i+1];
	  float y1w_r = y1_r * weight_real - y1_i * weight_imag;
	  float y1w_i = y1_r * weight_imag + y1_i * weight_real;
	  //if(n==64) printf("input val: (%f,%f)\n",y0_r,y0_i);
	  
	  results[i] = y0_r + y1w_r;
	  results[i + 1] = y0_i + y1w_i;
	  
	  results[n + i] = y0_r - y1w_r;
	  results[n + i + 1] = y0_i - y1w_i;
	}
      
      for (i = 0; i < 2 * n; i++)
	{
	  float junk;
	  get(float)(in,&junk);
	  //if(n==64) printf("putting: (%f)\n",results[i]);
	  put(float)(out,&results[i]);
	}
      
    }
  return 1;
}


void TASKDESTROY(){
  //printf("Combine, I ran %i proper times %i values in buffer\n",runtimes, length(float)(in));
  free(w);
  w = NULL;
}
  
