#include <stddef.h>

#ifndef LINK_H
#define LINK_H

// Forward declarations for links
struct task;
typedef struct task task_t;
typedef task_t* task_tp;

#define STRUCT_T task_tp
#include <structure.h>
#define DONE_task_tp 1

#define ARRAY_T task_tp
#include <array.h>
#define DONE_array_task_tp 1

#ifndef INNER_LINK_BUFFER
#define INNER_LINK_BUFFER void
#endif

struct link
{
	task_t *prod, *cons;
	INNER_LINK_BUFFER* buffer;
};
typedef struct link link_t;
typedef link_t* link_tp;

#define STRUCT_T link_tp
#include <structure.h>
#define DONE_link_tp 1

#define ARRAY_T link_tp
#include <array.h>
#define DONE_array_link_tp 1

#endif // LINK_T
