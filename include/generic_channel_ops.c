#include "communication.h"  
#include <stdio.h>

int get(DATATYPE)(channel_t inputchannel, DATATYPE *data)
{ 
  return get_bytes(inputchannel,(void*) data, sizeof(DATATYPE));
}


int put(DATATYPE)(channel_t outputchannel, DATATYPE *data)
{  
  //printf("::Putting %i\n",*(int*)data);
  return put_bytes(outputchannel,data,sizeof(*data));
}
int peek(DATATYPE)(channel_t inputchannel,size_t offset, DATATYPE *data)
{  
  return peek_bytes(inputchannel,offset,data,sizeof(*data));
}
size_t length(DATATYPE)(channel_t channel)
{
  int length_b = length_bytes(channel);
  /*
  if( length_b % sizeof(DATATYPE) != 0)
    {
      printf("Channel error: size of data in buffer is not multiple of datatype size.\n");
      exit(12);
    }
  */
  return length_bytes(channel) / sizeof(DATATYPE);
}
size_t capacity(DATATYPE)(channel_t channel)
{
  return capacity_bytes(channel) / sizeof(DATATYPE);
}

size_t  move(DATATYPE)(channel_t from, channel_t to, size_t max)
{
  return move_bytes(from, to, max*sizeof(DATATYPE));
}

void discard(DATATYPE)(channel_t channel,size_t numel)
{
  discard_bytes(channel,numel*sizeof(DATATYPE));
}
size_t fill(DATATYPE)(channel_t channel,size_t numel)
{
  return fill_bytes(channel,numel*sizeof(DATATYPE))/sizeof(numel);
}
