// This is the interface for communication handling. 
// These are backend functions. It is not aware of tasks, 
// and tasks does not have access to these functions.

#ifndef COMMUNICATION_BACKEND_H
#define COMMUNICATION_BACKEND_H
#include <stdlib.h>
#include "snekkja.h"
#include "task.h"

// channels data: {source_core,source_ordering,dest_core,dest_ordering} .. {..}
// Provided for preallocation purposes
int communication_setup(int this_core, const size_t (*channels)[2][2], int number_of_channels, int *argc, char ***argv);

// Free all memory. Return to a state where communication can be setup again.
void communication_destroy(void);

void global_barrier(void);

// Build a fifo channel between two tasks. These tasks can be local, or run on different cores.
// This function should be called only once per channel and core.
int build_channel(channel_t *channel,int this_core, int source_core, int source_ordering, int dest_core, int dest_ordering, task_status_t* prod_status, char* producer_taskid, char* consumer_taskid);

int build_inchannel(channel_t *channel,  char* filename, task_status_t *constant_status);
int build_outchannel(channel_t *channel, char* filename);
int flush_to_file(channel_t channel);
// Check what the other end has done with the channels and 
// update some meta values, if neccesary
int check_channels(channelc_t incoming_channels, channelc_t outgoing_channels);

// Notify the other end that data har been pushed or discarded
int update_channels(channelc_t inputchannels, channelc_t outputchannels, task_t* active_task);
int notify_state(channelc_t outputchannels, task_t* task);
int init_channel_collection(channelc_t *channel_collection, int capacity);
// Add one initialized channel to the channel collection.
int add(channelc_t channel_collection,channel_t channel);
// Check that all channels has data waiting
int is_dataready(channelc_t inputchannels);

unsigned int get_core_id(void);
// this will be called from the frontend (but is hidden from aut developer, forcing usage of
// type safe functions
int get_bytes(channel_t channel, void* data, size_t bytes);
int put_bytes(channel_t channel, void* data, size_t bytes);
int peek_bytes(channel_t hannel,size_t offset, void* data,size_t bytes);
size_t move_bytes(channel_t from, channel_t to, size_t max);
size_t fill_bytes(channel_t from, size_t max);
size_t discard_bytes(channel_t from, size_t max);
size_t length_bytes(channel_t channel);
size_t capacity_bytes(channel_t channel);
#endif
