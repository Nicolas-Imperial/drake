#include <stddef.h>

#ifndef ARCHITECTURE_H
#define ARCHITECTURE_H

int snekkja_arch_init(void*);
int snekkja_arch_finalize(void*);
volatile void* snekkja_arch_alloc(size_t, int);
int snekkja_arch_free(volatile void*);
int snekkja_arch_pull(volatile void*);
int snekkja_arch_commit(volatile void*);
int snekkja_core();
void snekkja_stderr(const char* format, ...);
void snekkja_stdout(const char* format, ...);
void snekkja_barrier(void*);

#endif // ARCHITECTURE_H
