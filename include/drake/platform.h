/*
 Copyright 2015 Nicolas Melot

 This file is part of Drake.

 Drake is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 Drake is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with Drake. If not, see <http://www.gnu.org/licenses/>.

*/

#include <stddef.h>
#include <stdio.h>

#ifndef DRAKE_PLATFORM_H
#define DRAKE_PLATFORM_H

#ifdef __cplusplus
extern "C"
{
#endif

struct drake_application;

/** Provides a frontend to create a stream for an application. Generates a call to drake_stream_create_explicit with a pointer to the function to schedule initialization function that corresponds to the application **/
#define drake_platform_stream_create(stream, application) \
drake_platform_stream_create_explicit(stream, PELIB_##CONCAT_2(drake_application_build_, application))

/** Abstract type for time measurement **/
typedef struct drake_time *drake_time_t;
/** Abstract type for time measurement **/
typedef struct drake_platform *drake_platform_t;
/** Abstract type for power measurement and recording **/
typedef struct drake_power *drake_power_t;
/** Memory descriptor **/
typedef struct drake_local_barrier *drake_local_barrier_t;

typedef enum drake_memory {DRAKE_MEMORY_UNDEFINED = 0, DRAKE_MEMORY_PRIVATE = 1, DRAKE_MEMORY_SHARED = 2, DRAKE_MEMORY_DISTRIBUTED = 3, DRAKE_MEMORY_SMALL_CHEAP = 4, DRAKE_MEMORY_LARGE_COSTLY = 8} drake_memory_t;
/** Returns the size in byte of the corresponding core's allocatable memory of a given type **/
size_t drake_platform_memory_size(unsigned int core, drake_memory_t type, unsigned int level);
/** Returns the size of a memory line in shared memory **/
//size_t drake_platform_shared_align();
size_t drake_platform_memory_alignment(unsigned int core, drake_memory_t type, unsigned int level);
/** Returns the size in byte of the calling core's allocatable private on-chip memory **/
//size_t drake_platform_private_size();
/** Initializes a drake stream execution platform. Runs before a streaming application is created **/
drake_platform_t drake_platform_init(void*);
/** Cleans up a drake stream execution platform. Runs after a streaming application is destroyed **/
int drake_platform_destroy(drake_platform_t);
/** Allocates on-chip communication memory for a given core. Memory allocated is only visible to the calling core. Every core should run this function in the same order using the same parameters. Any implementation must be deterministic.
	@param size Sice in byte of the memory to be allocated
	@param core id of the core for which to allocate memory
	@return A pointer to memory whose content is sent to corresponding core upon call to drake_platform_commit()
**/
//volatile void* drake_platform_shared_malloc(size_t size, size_t core);
//volatile void* drake_platform_shared_malloc_mailbox(size_t size, size_t core);

void* drake_platform_malloc(size_t size, unsigned int core, drake_memory_t type, unsigned int level);
void* drake_platform_calloc(size_t nmemb, size_t size, unsigned int core, drake_memory_t type, unsigned int level);
void* drake_platform_aligned_alloc(size_t alignment, size_t size, unsigned int core, drake_memory_t type, unsigned int level);
void drake_platform_free(void *ptr, unsigned int core, drake_memory_t type, unsigned int level);
struct drake_application;
struct drake_application* drake_platform_get_application_details();

/** Deallocates the on-chip communication memory corresponding to the address given. Only the calling cores can see that the memory has been freed. Every core should run this function in the same order using the same parameter. Any implementation must be deterministic. **/
//void drake_platform_shared_free(volatile void* addr, size_t core);
/** Allocates memory on on-chip private memory of the core that calls this function **/
//void* drake_platform_store_malloc(size_t size);
/** Frees on-chip private memory of the core that call this function **/
//void drake_platform_store_free(void*);
/** Allocate memory in off-chip private memory of the caller core **/
//void* drake_platform_private_malloc(size_t size);
/** Free memory in off-chip private memory of the caller core **/
//void drake_platform_private_free(void*);
/** Returns the memory address in a distant core, that corresponds to an address of the caller core's local communication memory and that can be used to send messages
	@param addr Address of the caller core's local communication memory
	@param core Core id that can directly read of write in the memory address returned
**/	
void* drake_remote_addr(void* addr, size_t core);
/** Fetch data from the core corresponding to the memory address **/
int drake_platform_pull(volatile void* addr);
/** Triggers the sending of data written at addr, to the corresponding core. The data may have been sent before the call, but it is guaranteed to have been sent after this function call. It is not guaranteed however, that the recicpient core has received it when the function returns. **/
int drake_platform_commit(volatile void* addr);
/** Returns the id of the core calling this function **/
unsigned int drake_platform_core_id();
/** Returns the number of cores available at the time the streaming program is run. This must match the number of cores in the schedule of the streaming task begin run **/
size_t drake_platform_core_size();
/** Maximum number of cores the execution platform can offer **/
size_t drake_platform_core_max();
/** Barrier across all cores involved in the streaming application running **/
void drake_platform_barrier(void*);
/** Allocates a barrier across cores sharing memory **/
drake_local_barrier_t drake_platform_local_barrier_alloc(unsigned int length, unsigned int core, drake_memory_t feature, unsigned int level);
/** Wait at local barrier **/
int drake_platform_local_barrier_wait(drake_local_barrier_t barrier);
/** Destroy local barrier **/
int drake_platform_local_barrier_destroy(drake_local_barrier_t barrier);
/** Begins a critical section **/
void drake_platform_exclusive_begin();
/** Ends a critical section **/
void drake_platform_exclusive_end();
/** Set the frequency of the caller core
	@param frequency Frequency in KHz
	@return 0 if frequency switching could not be performed
**/
int drake_platform_set_frequency(int frequency /* in KHz */);
/** Sets the frequency of the caller core and scales voltage if necessary
	@param frequency Frequency in KHz
	@return 0 if frequency switching could not be performed
**/
int drake_platform_set_frequency_autoscale(int frequency);
/** Sets the voltage of the caller core
	@param voltage Voltage in Volts
	@return 0 if voltage scaling could not be performed
**/
int drake_platform_set_voltage(float voltage);
/** Sets the frequency of the caller core and scales voltage if necessary
	@param frequency Frequency in KHz
	@return 0 if frequency switching could not be performed
**/
int drake_platform_set_voltage_frequency(drake_platform_t, size_t frequency);
/** Returns the current frequency in KHz **/
size_t drake_platform_get_frequency(drake_platform_t);
/** Returns the current voltage in Volts **/
float drake_platform_get_voltage();
/** Gives the current time in millisecond
	@param time Abstract type that receives the current time
	@return 0 if the time could not be measured
**/
int drake_platform_time_get(drake_time_t);
/** Substract two time quantities and stores the result in a time structure
	@param res Time structure containing the result of t1 - t2
	@param t1 First operand of time subtraction
	@param t2 Second operation of time subtraction
	@return 0 if subtraction could not be performed
**/
int drake_platform_time_subtract(drake_time_t res, drake_time_t t1, drake_time_t t2);
/** Adds two time quantities and stores the resutl in a time structure
	@param res Time struction containing the result of t1 + t2
	@param t1 First operand of the time addition
	@param t2 Second operand of the time addition
	@return 0 if addition could not be performed
**/
int drake_platform_time_add(drake_time_t res, drake_time_t t1, drake_time_t t2);
/** Returns 0 if t1 <= t2 **/
int drake_platform_time_greater(drake_time_t t1, drake_time_t t2);
/** Returns 0 if t1 != t2 **/
int drake_platform_time_equals(drake_time_t t1, drake_time_t t2);
/** Inittializes a time structure
	@param ms Time in millisecond to initialize time structure with **/
int drake_platform_time_init(drake_time_t t, double ms);
/** Writes time on output stream from time structure **/
FILE* drake_platform_time_printf(FILE* stream, drake_time_t);
/** Cleans up and deallocates a time structure **/
void drake_platform_time_destroy(drake_time_t time);
/** Allocates a time structure **/
drake_time_t drake_platform_time_alloc();
/** Makes the caller core to sleep
	@param period sleep time in milliseconds
	@return 0 if the core could not be put to sleep
**/
int drake_platform_sleep(drake_time_t period);
/** Descriptor of power quantities to manipulate. DRAKE_POWER_CHIP denotes the pwoer consumption of the whoe chip the caller core is part of. DRAKE_POWER_MEMORY_CONTROLLER is the power consumption of all memory controllers. DRAKE_POWER_CORE is the power of the caller core alone **/
enum drake_power_monitor {DRAKE_POWER_CHIP, DRAKE_POWER_MEMORY_CONTROLLER, DRAKE_POWER_CORE};
/** Initializes a power structure
	@param sample Maximal number of sample the power structure can hold 
	@param measurement binary combination of power quantities to measure. For instance, 1 << DRAKE_POWER_CHIP | 1 << DRAKE_POWER_CORE
	@return 0 if the power structure could not be allocated
**/
drake_power_t drake_platform_power_init(drake_platform_t pt, size_t samples, int measurement);
/** Begins power consumption measurement
	@param pwr Power data structure that holds power data measured and well as all pwoer measurement settings
**/
void drake_platform_power_begin(drake_power_t pwr);
/** Stops power consumption measurement **/
size_t drake_platform_power_end(drake_power_t);
/** Writes all power quantities measured associated to a time step, in output stream in human-readable manner, one line per time step, all power quantities of a time step in the same line, with a separator
	@param stream Output stream
	@param separator String to write between two power quantities shown in each output line
	@return Output stream after writing pwoer information
**/
FILE* drake_platform_power_printf(FILE* stream, drake_power_t, char* separator);
/** Writes a unique line of all power quantities measured associated to a time step, in output stream in human-readable manner, one line per time step, all power quantities of a time step in the same line, with a separator
	@param stream Output stream
	@param line line index, starting at 0, of the line to be written
	@param separator String to write between two power quantities shown in each output line
	@return Output stream after writing pwoer information
**/
FILE* drake_platform_power_printf_line(FILE* stream, drake_power_t, size_t line, char* separator);
/** Writes the sum of all power quantities measured associated to a time step, in output stream in human-readable manner, one line per time step, all power quantities of a time step in the same line, with a separator. 
	@param stream Output stream
	@param metrics Power quantities to be combined together, for instance 1 << DRAKE_POWER_CHIP | 1 << DRAKE_POWER_CORE
	@param separator String to write between two power quantities shown in each output line
	@return Output stream after writing pwoer information
**/
FILE* drake_platform_power_printf_cumulate(FILE* stream, drake_power_t, int metrics, char *separator);
/** Writes the sum of all power quantities measured associated to a time step, in output stream in human-readable manner, one line per time step, all power quantities of a time step in the same line, with a separator. 
	@param stream Output stream
	@param line line index, starting at 0, of the line to be written
	@param metrics Power quantities to be combined together, for instance 1 << DRAKE_POWER_CHIP | 1 << DRAKE_POWER_CORE
	@param separator String to write between two power quantities shown in each output line
	@return Output stream after writing pwoer information
**/
FILE* drake_platform_power_printf_line_cumulate(FILE* stream, drake_power_t, size_t line, int metrics, char *separator);
/** Clean up and deallocate the memory associated to a power measurement and recording structure **/
void drake_platform_power_destroy(drake_power_t);

int drake_platform_stream_create_explicit(drake_platform_t stream, struct drake_application*(*get_app)());
int drake_platform_stream_init(drake_platform_t stream, void* arg);
int drake_platform_stream_run(drake_platform_t);
int drake_platform_stream_destroy(drake_platform_t);

void drake_platform_core_disable(drake_platform_t pt, size_t core);
void drake_platform_core_enable(drake_platform_t pt, size_t core);
void drake_platform_sleep_disable(drake_platform_t pt, size_t core);
void drake_platform_sleep_enable(drake_platform_t pt, size_t core);

void drake_platform_stream_run_async(drake_platform_t);
int drake_platform_stream_wait(drake_platform_t);

#ifdef __cplusplus
}
#endif
#endif // DRAKE_PLATFORM_H

