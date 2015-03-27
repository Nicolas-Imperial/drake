#include <stdlib.h>
#include <string.h>

#include <processor.h>
#include <architecture.h>

processor_t*
pelib_alloc_collection(processor_t)(size_t size)
{
  int i, res;
  processor_t *proc;

  proc = malloc(sizeof(processor_t) + sizeof(task_t*) * size);
  res = 1;

  if (proc != NULL)
    {
      proc->handled_nodes = 0;
      proc->node_capacity = size;

      for (i = 0; i < proc->node_capacity; i++)
        {
          proc->task[i] = malloc(sizeof(task_t));
          res = res && (proc->task[i] != NULL);
        }

      if (res == 1)
        {
          return proc;
        }
    }

  // TODO: also deallocate all allocated tasks to proc
  free(proc);
  return NULL;
}

int
pelib_free(processor_t)(processor_t * proc)
{
  size_t i;

  for (i = 0; i < proc->node_capacity; i++)
    {
      free(proc->task[i]);
    }
  free(proc);

  return 1;
}

int
pelib_copy(processor_t)(processor_t source, processor_t * copy)
{
  size_t i;

  //copy = pelib_alloc_collection(processor_t)(source.node_capacity);
  copy->id = source.id;
  copy->source = source.source;
  copy->sink = source.sink;

  for (i = 0; i < source.handled_nodes; i++)
    {
      pelib_processor_insert_task(copy, source.task[i]);
    }

  

  return 1;
}

char*
pelib_string(processor_t)(processor_t proc)
{
  size_t i;
  char *str_p, *str_node;
  char *str;

  str = malloc(sizeof(char) * (PELIB_MAPPING_PROC_ID_CHAR_LENGTH
      + (sizeof(PELIB_MAPPING_SEPARATOR) + PELIB_MAPPING_NODE_CHAR_LENGTH)
          * proc.handled_nodes + 1));

  sprintf(str, "%u", proc.id);
  str_p = &(str[strlen(str)]);

  for (i = 0; i < proc.handled_nodes; i++)
    {
      str_node = pelib_string(task_tp)(proc.task[i]);
      sprintf(str_p, "%c%s", PELIB_MAPPING_SEPARATOR, str_node);
      free(str_node);
      str_p = &(str[strlen(str)]);
    }

  return str;
}

// Processor human interface
int
pelib_printf(processor_t)(processor_t proc)
{
  char *str = pelib_string(processor_t)(proc);

  snekkja_stdout("%s\n", str);
  free(str);

  return 1;
}

int
pelib_processor_insert_task(processor_t * proc, task_t * task)
{
  if (!(pelib_processor_find_task(proc, task->id) < proc->handled_nodes))
    {
      //printf("%s: task %d\n", __FUNCTION__, task->id);
      *(proc->task[proc->handled_nodes]) = *task;
      proc->task[proc->handled_nodes]->core = proc;
      proc->handled_nodes++;

      return 1;
    }
  else
    {
      return 0;
    }
}

int
pelib_processor_remove_task(processor_t * proc, task_id task)
{
  size_t index;
  task_t * pivot;

  index = pelib_processor_find_task(proc, task);
  if (index < proc->handled_nodes)
    {
      pivot = proc->task[index];
      proc->task[index]->core = NULL;
      proc->task[index] = proc->task[proc->handled_nodes - 1];
      proc->task[proc->handled_nodes - 1] = pivot;

      proc->handled_nodes--;

      return 1;
    }
  else
    {
      return 0;
    }
}

// Processors properties
size_t
pelib_processor_find_task(processor_t * proc, task_id task)
{
  size_t i;
  task_t * p_node;

size_t r = proc->handled_nodes;
  for (i = 0; i < r; i++)
    {
      p_node = proc->task[i];
      if (p_node->id == task)
        {
          return i;
        }
    }

  return proc->handled_nodes;
}

