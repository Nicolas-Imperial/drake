#ifndef SCC
#define SCC
#endif
#include "communication.h"
#include <stdio.h>


#include <RCCE.h>
#include <RCCE_lib.h>
//#include <sync.h>
//#include <printf.h>
#include <pelib_scc.h>
#include <pelib.h>
//#define printf pelib_scc_printf
//#include <scc_printf.h>
#include <monitor.h>
//#include <scc_integer.h>
#include <integer.h>
//#include <task.h>

//#define CFIFO_T char
//#include <scc_fifo.h>
//#define CFIFO_T char
//#include <scc_fifo.c>

#define INNER_LINK_BUFFER cfifo_t(int) //cfifo_t(char)
#define INPUT_LINK_BUFFER enum task_status
#define OUTPUT_LINK_BUFFER enum task_status
#define JOHJA_DEF
#include <mapping.h>
#ifndef JOHJA_PELIB
#error Incorrect version of pelib is linked
#endif

#undef INNER_LINK_BUFFER
#undef INPUT_LINK_BUFFER
#undef OUTPUT_LINK_BUFFER

#ifndef MPB_SIZE
#define MPB_SIZE 8128
//#define MPB_SIZE 4064

#endif
#define INNER_BUFFER_SIZE 65536
//#define INNER_BUFFER_SIZE 131072
//#define INNER_BUFFER_SIZE 262144
//#define INNER_BUFFER_SIZE 524288
//#define OUTPUT_BUFFER_SIZE 65536

#define MAX_CORES_SCC 48
#define MARKER 0xDEADBEEF
#define UNUSED(x) (void)(x)

// private implementation
struct channel
{
  link_t *link;
  cross_link_t *cross_link;
  char *producer_taskid;
  char *consumer_taskid;

  // status of the producer:
  // This is either a ptr to mbp, in which case status is updated with every push_link call
  // or, in the case of a local producer, it points directly into the task_t and does not
  // need updating. (If producer is an external source, status is set to zombie and then never touched)
  volatile task_status_t* status;
  task_status_t status_cached;
};
#include <monitor.h>
 //Maybe
#define STRUCT_T channel_t
#include <structure.h>
#define DONE_channel_t
/*
#define STRUCT_T char
#include <structure.h>
#define DONE_char_t
*/

/*
#define STRUCT_T int
#include <structure.h>
#define DONE_int_t*/

int pelib_copy(channel_t)(channel_t s1, channel_t * s2)
  {
    *s2 = s1;

    return PELIB_SUCCESS;
  }

int
pelib_init(channel_t)(channel_t *val)
  {
    UNUSED(val);
    return PELIB_SUCCESS;
  }

int
pelib_compare(channel_t)(channel_t a,channel_t b)
  {
    UNUSED(a);
    UNUSED(b);
    return 0;
  }

char*
pelib_string(channel_t)(channel_t a)
  {
    UNUSED(a);
    char str[] = "TOSTR NOT IMPLEMENTED";

    char *str2 =(char*) malloc(sizeof(str));
    sprintf(str, "%s", str);

    return str2;
  }

char*
pelib_string_detail(channel_t)(channel_t a, int level)
{
  UNUSED(level);
  return pelib_string(channel_t)(a);
}

size_t
pelib_fread(channel_t)(channel_t* buffer, size_t size, size_t nmemb, FILE* stream)
{
  UNUSED(buffer);
  UNUSED(size);
  UNUSED(nmemb);
  UNUSED(stream);
  return 0;
}
#define ARRAY_T channel_t
#include <array.h>
#define ARRAY_T channel_t
#include "array.c" // Ugly?
#undef ARRAY_T
#define DONE_array_channel_t
/*
#define ARRAY_T char
#include <array.h>
#define ARRAY_T char
#include "pelib_generic_datatype_impls.c"
#include "array.c" // Ugly?
#define CFIFO_ARRAY_T char
#define ARRAY_T char
#include <scc_fifo_array.h>
#define CFIFO_ARRAY_T char
#include <scc_fifo_array.c>


*/
/*
#define ARRAY_T int
#include <array.h>
#define ARRAY_T int
#include "pelib_generic_datatype_impls.c"
#include "array.c" // Ugly?
#define CFIFO_ARRAY_T int
#define ARRAY_T int
#include <scc_fifo_array.h>
#define CFIFO_ARRAY_T int
#include <scc_fifo_array.c>

#define DONE_array_int_t
*/

struct channel_collection
{
  array_t(channel_t)* channels;
};

static void check_output_link(task_t *task, cross_link_t *link);
static void check_input_link(task_t *task, cross_link_t *link);
static void feedback_link(task_t *task, cross_link_t *link);
static void push_link(task_t *task, cross_link_t* link);


// Return true if all inputchannels contain data
int is_dataready(channelc_t inputchannels)
{
  size_t i;
  size_t length = pelib_array_length(channel_t)(inputchannels->channels);
  for(i = 0; i < length; i++)
    {
      channel_t channel = pelib_array_read(channel_t)(inputchannels->channels,i); 
      if(channel->cross_link != NULL)
	{  
	  check_input_link(NULL,channel->cross_link);
	}
      if (pelib_array_length(int)(channel->link->buffer) == 0)
	{
	  return 0;
	}
    }
  return 1;
}
int check_channels(channelc_t inputchannels, channelc_t outputchannels)
{
  channel_t channel;
  size_t i,length;
  

  length = pelib_array_length(channel_t)(inputchannels->channels);
  for(i = 0; i < length; i++)
    {
      channel = pelib_array_read(channel_t)(inputchannels->channels,i); 
      RC_cache_invalidate();
      channel->status_cached =  *channel->status;
      if(channel->cross_link != NULL)
	{
	  check_input_link(NULL,channel->cross_link);
	}
    }
  length = pelib_array_length(channel_t)(outputchannels->channels);
  for(i = 0; i < length; i++)
    {
      channel = pelib_array_read(channel_t)(outputchannels->channels,i);
      if(channel->cross_link != NULL)
	{
	  check_output_link(NULL,channel->cross_link);
	}
    }
  return 0;
}

int update_channels(channelc_t inputchannels, channelc_t outputchannels, task_t* active_task)
{
  size_t i,length;
  channel_t channel;
  
  length = pelib_array_length(channel_t)(inputchannels->channels);
  for(i = 0; i < length; i++)
    {
      channel = pelib_array_read(channel_t)(inputchannels->channels,i);
      if(channel->cross_link != NULL)
	{
	  feedback_link(NULL,channel->cross_link);
	}
    }

  length = pelib_array_length(channel_t)(outputchannels->channels);
  for(i = 0; i < length; i++)
    {
      channel = pelib_array_read(channel_t)(outputchannels->channels,i);
      if(channel->cross_link != NULL)
	{
#ifdef DEBUG
	  if(channel->producer_taskid == NULL) exit(25);

	  if(strcmp(active_task->taskid, channel->producer_taskid) != 0)
	    {
	      printf("Channel error: pushing on someone elses channel. task: %s, channel producer: %s\n",active_task->taskid,channel->producer_taskid);
	      exit(40);
	    }
#endif
	  push_link(active_task,channel->cross_link);
	}
    }

  return 0;
}
int notify_state(channelc_t outputchannels, task_t* task){
  size_t i, length;

  length = pelib_array_length(channel_t)(outputchannels->channels);
  for(i = 0; i < length; i++)
    {
      channel_t channel = pelib_array_read(channel_t)(outputchannels->channels,i);
      if(channel->cross_link != NULL)
	{
	   RC_cache_invalidate();
	   *channel->cross_link->prod_state= task->status;
	   pelib_scc_force_wcb();
	}
    }
  return 0;
}
static void check_output_link(task_t *task, cross_link_t *link)
{
  UNUSED(task);
  //enum task_status new_state;
  
  RC_cache_invalidate();
  
  // Update input fifo length
  
  size_t read = *link->read - link->total_read;
  
  //size_t actual_read = pelib_cfifo_discard(char)(link->link->buffer, read);
  size_t actual_read = pelib_cfifo_discard(int)(link->link->buffer, read);
  link->actual_read = actual_read;
  
  if(actual_read > 0)
    {
      RC_cache_invalidate();
      // Keep track of how much was written before work
      //link->available = pelib_cfifo_length(char)(*(cfifo_t(char)*)link->link->buffer);
      link->available = pelib_cfifo_length(int)(*(cfifo_t(int)*)link->link->buffer);
      link->total_read += read;
    }
}

// update the link on how much I have read from it, if any
static void feedback_link(task_t *task, cross_link_t *link)
{
  UNUSED(task);
  //enum task_status new_state;
  size_t size;
  
  RC_cache_invalidate();
  //size = link->available - pelib_cfifo_length(char)(*(cfifo_t(char)*)link->link->buffer);
  size = link->available - pelib_cfifo_length(int)(*(cfifo_t(int)*)link->link->buffer);
  
  if(size > 0)
    {
      RC_cache_invalidate();
      
      link->total_read += size;
      RC_cache_invalidate();
      *link->read = link->total_read;
      pelib_scc_force_wcb();
      //link->available = pelib_cfifo_length(char)(*(cfifo_t(char)*)link->link->buffer);
      link->available = pelib_cfifo_length(int)(*(cfifo_t(int)*)link->link->buffer);
      
    }
}
// update the link on the new data written to it, if any. Also, update the task status info.
static void push_link(task_t *task, cross_link_t* link)
{
  RC_cache_invalidate();
  //size_t length = pelib_cfifo_length(char)(*(cfifo_t(char)*)link->link->buffer);
  size_t length = pelib_cfifo_length(int)(*(cfifo_t(int)*)link->link->buffer);
  size_t size = length - link->available;
  if(size > 0)
    {
      RC_cache_invalidate();
      
      link->total_written += size;
      *link->write = link->total_written;
      pelib_scc_force_wcb();// __sync_synchronize(); //DEBUG
      //*link->prod_state = task->status;
      //pelib_scc_force_wcb();
      //link->available = pelib_cfifo_length(char)(*(cfifo_t(char)*)link->link->buffer);
      link->available = pelib_cfifo_length(int)(*(cfifo_t(int)*)link->link->buffer);
    }
  else
    { //we still might just have gone from running to something else
      //printf("state address : %x\n",task);
      //RC_cache_invalidate();
      //*link->prod_state = task->status;
      //pelib_scc_force_wcb();
    }
}


static void check_input_link(task_t *task, cross_link_t *link)
{
  UNUSED(task);
  //enum task_status new_state;
  RC_cache_invalidate();
  // Update input fifo length
  size_t write = *link->write - link->total_written; 
  //size_t actual_write = pelib_cfifo_fill(char)(link->link->buffer, write);        
  size_t actual_write = pelib_cfifo_fill(int)(link->link->buffer, write);        
  link->actual_written = actual_write;

  if(actual_write > 0)
    {       
      RC_cache_invalidate();

      //link->available = pelib_cfifo_length(char)(*(cfifo_t(char)*)link->link->buffer); 
      link->available = pelib_cfifo_length(int)(*(cfifo_t(int)*)link->link->buffer); 
      link->total_written += write;  
    }
}

///// channel_aut implementation

channel_t get_channel(channelc_t c,int index)
{
  return pelib_array_read(channel_t)(c->channels,index);
}

unsigned int get_core_id(void)
{
  return  RCCE_ue();
}

int is_full(channel_t channel)
{
#ifdef DEBUG
  if(channel == NULL) exit(39);
#endif
  /*
  if(channel->cross_link != NULL)
    {
      check_output_link(NULL,channel->cross_link);
    }
  */
  //int is_full = pelib_cfifo_is_full(char)(channel->link->buffer);
  int is_full = pelib_cfifo_is_full(int)(channel->link->buffer);
  return is_full;
}
int is_empty(channel_t channel)
{
  //int empty = pelib_cfifo_is_empty(char)(channel->link->buffer);
  int empty = pelib_cfifo_is_empty(int)(channel->link->buffer);
#ifdef DEBUG
  int length = length_bytes(channel);
  if((empty && length != 0) || (!empty && length == 0))
    {
      printf("Channel Error: empty=:%i but length=%i ",empty,length);fflush(stdout);
      exit(40);
    }
#endif
  //return pelib_cfifo_is_empty(char)(channel->link->buffer);
  return pelib_cfifo_is_empty(int)(channel->link->buffer);
}

int is_alive(channel_t channel)
{
#ifdef DEBUG
  if(channel->status == NULL) exit(31);
#endif
  //task_status_t status = *channel->status;//channel->status_cached;
  task_status_t status = channel->status_cached;
#ifdef DEBUG
  if (status == TASK_INVALID) exit(32);
#endif
  //RC_cache_invalidate();
  return !(status == TASK_KILLED || status==TASK_ZOMBIE);
  //status == TASK_RUN || status == TASK_NEW ||status == TASK_INIT;// || status == TASK_KILLED;
}

const char* souce_taskid(channel_t channel){ UNUSED(channel); return "UNKNOWN";} //TODO: Implement
const char* dest_taskid(channel_t channel) { UNUSED(channel); return "UNKNOWN";}

size_t length_bytes(channel_t channel)
{
  //return pelib_cfifo_length(char)(*channel->link->buffer);
  return pelib_cfifo_length(int)(*channel->link->buffer)*sizeof(int);
}

size_t move_bytes(channel_t from, channel_t to, size_t max_bytes)
{
  return pelib_cfifo_popfifo(int)(from->link->buffer,to->link->buffer,max_bytes/sizeof(int));
}
size_t fill_bytes(channel_t channel, size_t bytes)
{
  return pelib_cfifo_fill(int)(channel->link->buffer,bytes*sizeof(int));
}
size_t get_buffer_read_offset(channel_t channel)
{
  return channel->link->buffer->read;
}
void discard_bytes(channel_t channel, size_t bytes)
{
  pelib_cfifo_discard(int)(channel->link->buffer,bytes/sizeof(int));
}
size_t capacity_bytes(channel_t channel)
{
  //return pelib_cfifo_capacity(char)(channel->link->buffer);
  return pelib_cfifo_capacity(int)(channel->link->buffer)*sizeof(int);
}
int get_bytes(channel_t channel, void* data, size_t bytes){
#ifdef DEBUG
  if(channel == NULL) exit (22);
  if(channel->link == NULL) exit(23);
  if(channel->link->buffer == NULL) exit(24);
  exit(20);
  if(data == NULL) exit(26);
  if(bytes == 0) exit(27);
#endif

  //size_t retval = pelib_cfifo_popmem(char)(channel->link->buffer,(char*)data,bytes);
  //  size_t retval = pelib_cfifo_popmem(int)(channel->link->buffer,(int*)data,bytes/sizeof(int));
  int* d = data;
#ifdef SCC
  int value = channel->link->buffer->buffer[channel->link->buffer->read];
  while(value ==  MARKER)
    {
      RC_cache_invalidate();
      value = channel->link->buffer->buffer[channel->link->buffer->read];
    }
  RC_cache_invalidate();
  channel->link->buffer->buffer[channel->link->buffer->read] = MARKER;
  pelib_cfifo_discard(int)(channel->link->buffer,1);
  *d = value;
#else
  *d = pelib_cfifo_pop(int)(channel->link->buffer);
#endif
  return 1;
}
void* get_fifoarr(channel_t channel)
{
  return channel->link->buffer->buffer;
}
int put_bytes(channel_t channel, void* data, size_t bytes){

  //size_t retval = pelib_cfifo_pushmem(char)(channel->link->buffer,(char*)data,bytes/sizeof(char));
  //size_t retval = pelib_cfifo_pushmem(int)(channel->link->buffer,(int*)data,bytes/sizeof(int));exit(1);
  size_t retval = pelib_cfifo_push(int)(channel->link->buffer,*(int*)data);//  exit(2);
#ifdef DEBUG
  if(retval == 0)
    {
      exit(28);
    }
#endif  
  return retval;
}

int peek_bytes(channel_t channel,size_t offset, void* data,size_t bytes)
{
  int* d = (int*) data;
#ifdef SCC
  int value =pelib_cfifo_peek(int)(channel->link->buffer,offset);
  while(value ==  MARKER) // if true, expensive. However, it only happens every one millionth time or so...
    {
      RC_cache_invalidate();
      value =pelib_cfifo_peek(int)(channel->link->buffer,offset);
    }
  *d = value;
#else
  *d =  pelib_cfifo_peek(int)(channel->link->buffer,offset);
#endif
  return 1;
}
///// end channel_aut implementation					 
static size_t buffer_size(size_t nb_in, size_t nb_out) //modified copy from pipelined_merge.c
{
  if(nb_in == 0)
    {
      return 0;
    }
  return (MPB_SIZE // total allocable MPB size
	  - nb_in // As many input buffer as
	  * (sizeof(size_t)  // Size of a write
	     + sizeof(enum task_status) // State of a task
	     )
	  - nb_out // As many output buffers as
	  * sizeof(size_t) // Size of a read
	  )
    /  nb_in; // Remaining space to be divided by number of input links
}

struct local_channel{
  int source_ordering;
  int dest_ordering;
  link_t *link;
};

static int capacity[MAX_CORES_SCC];
static int inner_buffer_capacity;
//static struct local_channel **local_channels = NULL;
//static scc_cfifo_init_t init;
static mpb_stack_t *on_die_memory_stack;
int communication_setup(int this_core, const size_t (*channels)[2][2], int number_of_channels, int *argc, char ***argv)
{

  UNUSED(this_core);
  int retval = pelib_scc_init(argc,argv);
  if(retval != 0 && retval != PELIB_SUCCESS)
    {
      printf("Error: RCCE init failed\n");
      return retval;
    }
  // Determine how large the buffers can be on each core
  int channelcount[MAX_CORES_SCC][2] = {{0,0}};   //source, destination
  int local_channelcount = 0;
  int i;

  for(i = 0; i < number_of_channels; i++)
    {
      if(channels[i][0][0] == channels[i][1][0])
	{
	  if(channels[i][1][0] == get_core_id()) // if both tasks are on this core
	    {
	      local_channelcount +=1;
	    }
	  //else ignore. local on another core.
	}
      else
	{
	  //inter-core channel
	  channelcount[ channels[i][0][0] ][1] += 1; // one more output(source) on this core
	  channelcount[ channels[i][1][0] ][0] += 1; // one more input (destination) on this core
	}
    }
  //local_channelcount /= 2; // All local are counted twice. 

  for(i = 0; i <MAX_CORES_SCC;i++)
    {
      //capacity[i] = buffer_size(channelcount[i][0],channelcount[i][1])/sizeof(char) / 2; 
      capacity[i] = buffer_size(channelcount[i][0],channelcount[i][1])/sizeof(int);// / 2; //how large can each buffer on respective core be 
      capacity[i] -= capacity[i] % 4; //TODO: NOT GENERIC!
      //#ifdef DEBUG
      if(get_core_id()==0)printf("*capacity on %i is %i bytes\n",i,capacity[i]*sizeof(int));
      //#endif
    }

  int local_channelcount_div = local_channelcount == 0 ? 1 : local_channelcount; // avoid division with zero
  //inner_buffer_capacity = INNER_BUFFER_SIZE / local_channelcount / sizeof(char);
  inner_buffer_capacity = INNER_BUFFER_SIZE / local_channelcount_div / sizeof(int);
 
  //printf("(%i)I get %i local channels, %i bytes per channel.\n",get_core_id(),local_channelcount,inner_buffer_capacity*sizeof(int));
  on_die_memory_stack =  pelib_scc_stack_malloc(MPB_SIZE);
  return 0;  
}

void communication_destroy(void)
{
  //printf ("stub: %s\n", __FUNCTION__);
}
static int count = 0;
void global_barrier(void)
{
  //printf("(%i) arriving at global barrier time number %i\n",get_core_id(),++count);
  //fflush(stdout);
  pelib_scc_global_barrier();
  //printf("(%i) leaving global barrier time number %i\n",get_core_id(),count);
  //fflush(stdout);
}
int build_inchannel(channel_t *channel,char* filename, task_status_t *constant_status)
{
  //printf("opening file at: %s\n", filename);
  //PELIB_SCC_CRITICAL_BEGIN
  //array_char_t* arr = pelib_array_char_loadfilenamebinary(filename);
  array_int_t* arr = pelib_array_int_loadfilenamebinary(filename);
//PELIB_SCC_CRITICAL_END
  //printf("Read went ok\n");

  //cfifo_char_t* fifo = pelib_cfifo_from_array(char)(arr);
  cfifo_int_t* fifo = pelib_cfifo_from_array(int)(arr);
  *channel = (channel_t) malloc(sizeof(struct channel));
  (*channel)->status = constant_status;
  (*channel)-> cross_link = NULL;

  (*channel)->link = (link_t*) malloc(sizeof(link_t));
  (*channel)->link->buffer = fifo;
  return 0;
}

#define SUPER_MEGA_BIG 33*1024*1024;
int build_outchannel(channel_t *channel,char* filename)
{
  //printf("Building an out channel to %s\n", filename);
  *channel = (channel_t) malloc(sizeof(struct channel));
  (*channel)->consumer_taskid = filename; //filename is stored at taskid slot 
  (*channel)-> cross_link = NULL;
  (*channel)->link = (link_t*) malloc(sizeof(link_t));

  //(*channel)->link->buffer = pelib_alloc(cfifo_t(char))(&init);
  int c = SUPER_MEGA_BIG;
  (*channel)->link->buffer = pelib_alloc(cfifo_t(int))((void*)c);

  //pelib_init(cfifo_t(char))((*channel)->link->buffer);
  pelib_init(cfifo_t(int))((*channel)->link->buffer);

  return 0;
}
#include <limits.h>
int flush_to_file(channel_t channel)
{
  //array_t(char)* arr = pelib_array_char_from_cfifo(channel->link->buffer);
  array_t(int)* arr = pelib_array_int_from_cfifo(channel->link->buffer);
  size_t i;
  int oldval = INT_MIN;
  /*
  for(i = 0; i < pelib_array_char_length(arr)/4; i++)
    {
      int val = ((int*)arr->data)[i];
      if(val < oldval)
	{
	  printf("fileflush: value number %i NOT sorted. %i < %i \n",i,val, oldval);
	  break;
	}
      oldval = val;
    }*/
  //printf("Size of flush: %i",pelib_array_int_length(arr));
  for(i = 0; i < pelib_array_int_length(arr); i++)
    {
      int val = (arr->data)[i];
      if(val < oldval)
	{
	  printf("fileflush: value number %u NOT sorted. %i < %i \n",i,val, oldval);
	  break;
	}
      oldval = val;
    }

  char *filename = channel->consumer_taskid;
  //pelib_array_storefilenamebinary(char)(arr, filename);
  pelib_array_storefilenamebinary(int)(arr, filename);
  return 0;
}
int build_channel(channel_t *a_channel,int this_core, int source_core, int source_ordering, int dest_core, int dest_ordering, task_status_t* prod_status, char* producer_taskid, char* consumer_taskid)
{
  if(source_core == dest_core && !((size_t)source_core == get_core_id())) return 1; // ignore channels local to other cores
  channel_t channel = (channel_t) malloc(sizeof(struct channel));
  *a_channel = channel;
  UNUSED(this_core);
  UNUSED(source_ordering);
  UNUSED(dest_ordering);


  channel->producer_taskid = producer_taskid;
  channel->consumer_taskid = consumer_taskid;

  // local channel
  if(source_core == dest_core)
    {
  
      channel->cross_link = NULL;

      channel->link = (link_t*) malloc(sizeof(link_t));
      //channel->link->buffer = pelib_alloc(cfifo_t(char))(&init);
      channel->link->buffer = pelib_alloc(cfifo_t(int))((void*)inner_buffer_capacity);

      //pelib_init(cfifo_t(char))(channel->link->buffer);
      pelib_init(cfifo_t(int))(channel->link->buffer);
      channel->status = prod_status;
      return 0;
    }

  //Not a local channel, do mpb stuff


  channel->link = (link_t*) malloc(sizeof(link_t));

  //build fifo on target mpb stack
  cfifo_t(int) *b = (cfifo_t(int)*) malloc(sizeof(cfifo_t(int)));

  b->capacity = capacity[dest_core];  
  b->buffer = pelib_scc_global_ptr(pelib_scc_stack_grow(on_die_memory_stack, sizeof(int) * b->capacity, dest_core), dest_core);

  
  channel->link->buffer = b;
  //printf("capacity on %i is %i\n", dest_core, init.capacity);
  //channel->link->buffer = pelib_alloc(cfifo_t(char))(&init);
  //pelib_init(cfifo_t(char))(channel->link->buffer);
  pelib_init(cfifo_t(int))(channel->link->buffer);

  channel->cross_link = (cross_link_t*) malloc(sizeof(cross_link_t));
  channel->cross_link->link = channel->link;
  channel->cross_link->total_read = 0;
  channel->cross_link->total_written = 0;
  channel->cross_link->available = 0;


  //allocate read variable on source mpb stack
  channel->cross_link->read = (volatile size_t*)
    pelib_scc_global_ptr(pelib_scc_stack_grow(on_die_memory_stack, sizeof(size_t),source_core),source_core);
  *channel->cross_link->read=0;

  //size_t end_buffer = (size_t)pelib_scc_global_ptr(pelib_scc_stack_grow(init.stack,0,dest_core),dest_core);

  //size_t aligner = 32 - (end_buffer % 32);
  //pelib_scc_stack_grow(init.stack,aligner,dest_core);

  // allocate write variable on destination mpb stack.
  channel->cross_link->write = (volatile size_t*)
    pelib_scc_global_ptr(pelib_scc_stack_grow(on_die_memory_stack,sizeof(size_t),dest_core),dest_core);
    //pelib_scc_global_ptr(pelib_scc_stack_grow(init.stack,sizeof(size_t)+sizeof(task_status_t),dest_core),dest_core);

  //"allocate" status variable for the producer on the destination mpb stack
  //channel->cross_link->prod_state = (task_status_t*) (channel->cross_link->write + 1);
  //printf("write is at %x, prod is at %x, diff= %x\n",(unsigned long) channel->cross_link->write,channel->cross_link->prod_state,(unsigned long)((long int)channel->cross_link->prod_state- (long int)channel->cross_link->write));


  //allocate status variable for the producer on the destination mpb stack
  channel->cross_link->prod_state = 
    pelib_scc_global_ptr(pelib_scc_stack_grow(on_die_memory_stack,sizeof(task_status_t),dest_core),dest_core);
  //
  //pelib_scc_stack_grow(init.stack,32,dest_core);

  channel->status=channel->cross_link->prod_state;

  
  *a_channel = channel;
  return 0;
}

int init_channel_collection(channelc_t *channel_collection, int capacity)
{
  *channel_collection = (channelc_t) malloc(sizeof(struct channel_collection));
  (*channel_collection)->channels = pelib_alloc(array_t(channel_t))((void*)(size_t)capacity);
  return 0;
}

int add(channelc_t channel_collection,channel_t channel)
{
  if(channel_collection == NULL || channel_collection->channels == NULL || channel == NULL) exit(31);
  if(pelib_array_append(channel_t)(channel_collection->channels,channel) != 1)
    {
#ifdef DEBUG
      printf("Doubling capacity on channel collection to %lu\n",(unsigned long) pelib_array_capacity(channel_t)(channel_collection->channels)*2);
#endif
      //array is full, increase capacity
      array_t(channel_t) *newarray =  pelib_alloc(array_t(channel_t))
	((void*)(pelib_array_capacity(channel_t)(channel_collection->channels)*2));

      pelib_copy(array_t(channel_t))(*channel_collection->channels,newarray);
      //pelib_free_buffer(array_t(channel_t))(channel_collection->channels);
      channel_collection->channels = newarray;
      pelib_array_append(channel_t)(channel_collection->channels,channel);
    }
  return 0;
}


