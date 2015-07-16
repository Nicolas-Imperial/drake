#include <stddef.h>

#ifndef DRAKE_PLATFORM_H
#define DRAKE_PLATFORM_H

typedef struct drake_time *drake_time_t;

size_t drake_arch_local_size();
size_t drake_arch_store_size();
int drake_arch_init(void*);
int drake_arch_finalize(void*);
volatile void* drake_arch_alloc(size_t, int);
int drake_arch_free(volatile void*);
int drake_arch_pull(volatile void*);
int drake_arch_commit(volatile void*);
size_t drake_core();
size_t drake_core_size();
void drake_stderr(const char* format, ...);
void drake_stdout(const char* format, ...);
void drake_barrier(void*);
void drake_exclusive_begin();
void drake_exclusive_end();
void* drake_local_malloc(size_t size);
void drake_local_free(void*);
void* drake_private_malloc(size_t size);
void drake_private_free(void*);
void* drake_remote_addr(void*, size_t);
int drake_arch_set_frequency(int frequency /* in KHz */);
int drake_arch_set_frequency_autoscale(int frequency /* in KHz */);
int drake_arch_set_voltage(float voltage /* in volts */);
int drake_arch_set_voltage_frequency(int frequency /* in KHz */);
int drake_arch_get_frequency(); /* in KHz */
float drake_arch_get_voltage(); /* in volts */
int drake_get_time(drake_time_t);
int drake_time_substract(drake_time_t res, drake_time_t t1, drake_time_t t2);
int drake_time_add(drake_time_t res, drake_time_t t1, drake_time_t t2);
int drake_time_greater(drake_time_t t1, drake_time_t t2);
int drake_time_equals(drake_time_t t1, drake_time_t t2);
int drake_time_init(drake_time_t t, double ms);
void drake_time_display(FILE* stream, drake_time_t);
void drake_time_destroy(drake_time_t time);
drake_time_t drake_time_alloc();
int drake_arch_sleep(drake_time_t period);

#define DRAKE_POWER_CORE 1
#define DRAKE_POWER_MEMORY_CONTROLLER 2

typedef struct drake_power *drake_power_t;

drake_power_t drake_platform_power_init(size_t samples, int measurement);
void drake_platform_power_begin(drake_power_t);
size_t drake_platform_power_end(drake_power_t);
void drake_platform_power_display(FILE* stream, drake_power_t, char* separator);
void drake_platform_power_display_line(FILE* stream, drake_power_t, size_t line, char* separator);
void drake_platform_power_destroy(drake_power_t);

#endif // DRAKE_PLATFORM_H
