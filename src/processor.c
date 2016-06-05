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


#include <stdlib.h>
#include <string.h>

#include <drake/processor.h>
#include <drake/platform.h>

processor_t*
pelib_alloc_collection(processor_t)(size_t size)
{
  int i, res;
  processor_t *proc;

  proc = malloc(sizeof(processor_t));
  res = 1;

  if (proc != NULL)
    {
      proc->handled_nodes = 0;
      proc->node_capacity = size;
      proc->task = malloc(sizeof(task_t*) * size);
      if(proc->task != NULL)
      {

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

      // If something went wrong, free all tasks and task array
      for (i = 0; i < proc->node_capacity; i++)
      {
        free(proc->task[i]);
      }
      free(proc->task);
    }

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
  free(proc->task);
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
      drake_processor_insert_task(copy, source.task[i]);
    }

  

  return 1;
}

char*
pelib_string(processor_t)(processor_t proc)
{
  size_t i;
  char *str_p, *str_node;
  char *str;

  str = malloc(sizeof(char) * (DRAKE_MAPPING_PROC_ID_CHAR_LENGTH
      + (sizeof(DRAKE_MAPPING_SEPARATOR) + DRAKE_MAPPING_NODE_CHAR_LENGTH)
          * proc.handled_nodes + 1));

  sprintf(str, "%u", proc.id);
  str_p = &(str[strlen(str)]);

  for (i = 0; i < proc.handled_nodes; i++)
    {
      str_node = pelib_string(task_tp)(proc.task[i]);
      sprintf(str_p, "%c%s", DRAKE_MAPPING_SEPARATOR, str_node);
      free(str_node);
      str_p = &(str[strlen(str)]);
    }

  return str;
}

// Processor human interface
FILE*
pelib_printf(processor_t)(FILE* stream, processor_t proc)
{
  char *str = pelib_string(processor_t)(proc);

  printf("%s", str);
  free(str);

  return stream;
}

int
drake_processor_insert_task(processor_t * proc, task_t * task)
{
  if (!(drake_processor_find_task(proc, task->id) < proc->handled_nodes))
    {
/*
      if(proc->task[proc->handled_nodes]->core != NULL)
      {
        processor_t **list = task->core;
        task = malloc(sizeof(processor_t*) * task->width + 1);
	memcpy(task->core, list, sizeof(processor_t*) * task->width);
        task->core[sizeof(processor_t*) * task->width] = proc;
        task->width++;
      }
      else
      {
        task->core = malloc(sizeof(processor_t*));
        task->core[0] = proc;
        task->width = 1;
      }
*/
      *(proc->task[proc->handled_nodes]) = *task;
      proc->handled_nodes++;

      return 1;
    }
  else
    {
      return 0;
    }
}

int
drake_processor_remove_task(processor_t * proc, task_id task)
{
  size_t index;
  task_t * pivot;

  index = drake_processor_find_task(proc, task);
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
drake_processor_find_task(processor_t * proc, task_id task)
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

