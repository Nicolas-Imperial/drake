#ifndef DATATYPE 
#define DATATYPE int //default
#endif

#ifndef SNEKKJA
#define SNEKKJA

#include <stdlib.h>
typedef struct channel* channel_t;
typedef struct channel_collection* channelc_t;
#define quotify(x) #x
#define xquotify(x) quotify(x)
// These are to be implemented by the task
// They will be called by the backend at runtime
int TASKSETUP(channelc_t incoming, channelc_t outgoing, int argc, char *argv[]);
int TASKPRERUN(void);
int TASKRUN(void);
void TASKDESTROY(void);

/// API for the task into the backend functions
channel_t get_channel(channelc_t channels,int index);
size_t size(channelc_t);

const char* souce_taskid(channel_t channel);
const char* dest_taskid(channel_t channel);

int is_full(channel_t channel);
int is_empty(channel_t channel);
int is_alive(channel_t channel);

void* get_fifoarr(channel_t channel);
size_t get_buffer_read_offset(channel_t channel);
int      RCCE_qsort(char *, size_t, size_t, int (*)(const void*, const void*));
//DATATYPE get_buffer(channel_t channel);
//void discard(DATATYPE)(channel_t channel, size_t numel);
//size_t capacity(DATATYPE)(channel_t channel);
//How meny elements reside in the buffer right now
/* I assure you, these exist

   size_t length(DATATYPE)(channel_t channel);
   int put(DATATYPE) (channel_t outputchannel, DATATYPE *data); 
   int get(DATATYPE) (channel_t outputchannel, DATATYPE *data); 
   size_t capacity(DATATYPE)(channel_t channel);


*/
//		       void discard(DATATYPE)(channel_t channel,size_t numel);

/// Implementation details. Nothing to look at. move along!

#include <template.h>

#define get(DATATYPE) PELIB_CONCAT_2(get_,DATATYPE)
#define put(DATATYPE) PELIB_CONCAT_2(put_,DATATYPE)
#define peek(DATATYPE) PELIB_CONCAT_2(peek_,DATATYPE)
#define length(DATATYPE) PELIB_CONCAT_2(length_,DATATYPE)
#define capacity(DATATYPE) PELIB_CONCAT_2(capacity_,DATATYPE)
#define move(DATATYPE) PELIB_CONCAT_2(move_,DATATYPE)
#define discard(DATATYPE) PELIB_CONCAT_2(discard_,DATATYPE)
#define fill(DATATYPE) PELIB_CONCAT_2(fill_,DATATYPE)

#define AUT_TASKNAME PELIB_CONCAT_3(AUT,_,TASKNAME)
#define AUT_TASKNAME_TASKID PELIB_CONCAT_3(AUT_TASKNAME,_,TASKID)
#define TASKSETUP PELIB_CONCAT_3(AUT_TASKNAME_TASKID,_,setup)
#define TASKPRERUN  PELIB_CONCAT_3(AUT_TASKNAME_TASKID,_,prerun)
#define TASKRUN     PELIB_CONCAT_3(AUT_TASKNAME_TASKID,_,run)
#define TASKDESTROY PELIB_CONCAT_3(AUT_TASKNAME_TASKID,_,destroy)

#ifndef COMMUNICATION_BACKEND_H

// Move one data packet from the fifo to 
// private memory
int __attribute__((weak)) get(DATATYPE)(channel_t inputchannel, DATATYPE *data);

// Send one data packet of the type DATATYPE to the fifo channel
int __attribute__((weak)) put(DATATYPE) (channel_t outputchannel, DATATYPE *data);
int __attribute__((weak)) peek(DATATYPE) (channel_t inputchannel,size_t offset, DATATYPE *data);
size_t __attribute__((weak)) length(DATATYPE)(channel_t channel);
size_t __attribute__((weak)) capacity(DATATYPE)(channel_t channel);
//Move directly from fifo to fifo
size_t __attribute__((weak)) move(DATATYPE)(channel_t from, channel_t to, size_t max);
void __attribute__((weak)) discard(DATATYPE)(channel_t channel,size_t numel);
size_t __attribute__((weak)) fill(DATATYPE)(channel_t channel,size_t numel);

#include "generic_channel_ops.c"

#endif
#endif
