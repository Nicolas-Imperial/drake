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

#include <drake/mapping.h>
#include <drake/platform.h>
#include <assert.h>

#include "drawgraph-2.h"

// Mapping manipulations
mapping_t*
pelib_alloc_collection(mapping_t)(size_t size)
{
  mapping_t * mapping;
  size_t res;

  res = 1;
  mapping = malloc(sizeof(mapping_t) + sizeof(processor_t*) * size);

  if (mapping != NULL)
    {
      mapping->proc = malloc(sizeof(processor_t*) * size);
      if(mapping->proc != NULL)
      {
      mapping->processor_count = 0;
      mapping->max_processor_count = size;
      mapping->task_count = 0;

      // Makes sure the allocated memory was actually allocated
      if (res != 0)
        {
          return mapping;
        }
      }
      free(mapping->proc);
      free(mapping);
    }

  return NULL;
}

int
pelib_free(mapping_t)(mapping_t * solution)
{
  size_t i;

  for (i = 0; i < solution->processor_count; i++)
    {
      pelib_free(processor_t)(solution->proc[i]);
    }
  free(solution->proc);
  free(solution);

  return 1;
}

int
pelib_copy(mapping_t)(mapping_t source, mapping_t *copy)
{
  size_t i;

  //copy = pelib_alloc(mapping_t)(DRAKE_MAPPING_MAX_TASK_COUNT);
  copy->processor_count = source.processor_count;
  copy->task_count = source.task_count;

  for (i = 0; i < copy->processor_count; i++)
    {
      //drake_processor_destroy(copy->proc[i]);
      pelib_copy(processor_t)(*source.proc[i], copy->proc[i]);
    }

  return 1;
}

int
drake_mapping_insert_task(mapping_t* mapping, processor_id pid, task_t* task)
{
  int processor_index;

  processor_index = drake_mapping_find_processor_index(mapping, pid);
  
  if (drake_processor_insert_task(mapping->proc[processor_index], task) == 1)
    {
      mapping->task_count++;

      return 1;
    }
  else
    {
      return 0;
    }
}
int
drake_mapping_remove_task(mapping_t* mapping, task_id task)
{
  processor_id pid;

  pid = drake_mapping_find_processor(mapping, task);

  if (pid < mapping->processor_count && drake_processor_remove_task(
      mapping->proc[pid], task) == 1)
    {
      mapping->task_count--;

      return 1;
    }
  else
    {
      return 0;
    }
}
int
drake_mapping_find_processor_index(mapping_t* mapping, processor_id pid)
{
  int i;
  for (i = 0; i < mapping->processor_count; i++)
    {
      if (mapping->proc[i]->id == pid)
        {
          return i;
        }
    }
  return i;
}
processor_id
drake_mapping_find_processor(mapping_t* mapping, task_id tid)
{
  int i;
  for (i = 0; i < mapping->processor_count; i++)
    {
      if (drake_processor_find_task(mapping->proc[i], tid)
          != mapping->proc[i]->handled_nodes)
        {
          return mapping->proc[i]->id;
        }
    }

  return -1;
}
int
drake_mapping_insert_processor(mapping_t* mapping, processor_t* processor)
{
  // If there is room for a new processor in mapping, and this processor id was not already present
  if (mapping->processor_count < mapping->max_processor_count
      && drake_mapping_find_processor_index(mapping, processor->id)
          == mapping->processor_count)
    {
	mapping->proc[mapping->processor_count] = pelib_alloc_collection(processor_t)(processor->node_capacity);

//      printf("%s: %X\n", __FUNCTION__, mapping->proc[mapping->processor_count]);

      if (mapping->proc[mapping->processor_count] != NULL)
        {
      pelib_copy(processor_t)(*processor, mapping->proc[mapping->processor_count]);
          mapping->processor_count++;

          return 1;
        }
      else
        {
          return 0;
        }
    }
  else
    {
      return 0;
    }
}
int
drake_mapping_remove_processor(mapping_t* mapping, int processor_id)
{
  // TODO: implement
  return 0;
}

// Mapping properties
int
drake_mapping_violations(mapping_t* mapping)
{
  // A mapping is correct iff every task is mapped to exactly one processor
  processor_id pid, found_pid;
  size_t i, mapped, overmapped;

  overmapped = 0;
  mapped = 0;

  for (pid = 0; pid < mapping->processor_count; pid++)
    {
      for (i = 0; i < mapping->proc[pid]->handled_nodes; i++)
        {
          found_pid = drake_mapping_find_processor(mapping,
              mapping->proc[pid]->task[i]->id);

          if (found_pid == pid) // All good
            {
              mapped++;
            }
          else
            {
              if (found_pid != mapping->processor_count) // // The task is mapped to several processors
                {
                  overmapped++;
                }
              // else the task is not mapped at all
            }
        }
    }

  if (mapped == mapping->task_count && overmapped == 0)
    {
      return 1;
    }
  else
    {
      return 0;
    }
}

// Mapping human interface
FILE*
pelib_printf(mapping_t)(FILE* stream, mapping_t mapping)
{
  char *str = pelib_string(mapping_t)(mapping);
  fprintf(stream, "%s\n", str);

  free(str);

  return stream;
}

char*
pelib_string(mapping_t)(mapping_t mapping)
{
  size_t i, total_length;
  char *proc_str, *str;

  total_length = 0;
  for (i = 0; i < mapping.processor_count; i++)
    {
      total_length += (DRAKE_MAPPING_PROC_ID_CHAR_LENGTH
          + (sizeof(DRAKE_MAPPING_SEPARATOR) + DRAKE_MAPPING_NODE_CHAR_LENGTH)
              * mapping.proc[i]->handled_nodes + 2);
    }
  total_length++; // Provide space for the trailing character \0

  str = malloc(sizeof(char) * total_length);
  str[0] = '\0';

  for (i = 0; i < mapping.processor_count; i++)
    {
      proc_str = pelib_string(processor_t)(*mapping.proc[i]);
      sprintf(&(str[strlen(str)]), "[%s]", proc_str);
      free(proc_str);
    }

  return str;
}

static int
take_all_tasks(task_t *task)
{
  return 1;
}

static mapping_t*
mapping_loadstr(mapping_t * mapping, char * str, int(filter)(task_t*))
{
  FILE * stream_str;

  //printf("[%s] %s\n", __func__, str);
  stream_str = (FILE*)fmemopen((void*)str, strlen(str), "r");
 // printf("file descriptor: %i\n", *stream_str);

  mapping = drake_mapping_loadfilterfile(mapping, stream_str, filter);
  fclose(stream_str);
  //printf("%i\n", i == EOF);

  return mapping;
}

mapping_t*
drake_mapping_loadstr(mapping_t * mapping, char * str)
{
  return mapping_loadstr(mapping, str, take_all_tasks);
}

mapping_t*
drake_mapping_loadfilterstr(mapping_t* mapping, char* str, int (filter)(task_t*))
{
  //printf("[%s] %s\n", __func__, str);
  return mapping_loadstr(mapping, str, filter);
}

/* TODO: draw mappings with as many tasks as in the mapping instead of 2 ^ (p - 1) */
static void
mapping_draw(FILE* read, FILE* write, FILE* err)
{
  pelib_drawgraph2_draw(read, write, err);
}

mapping_t*
drake_mapping_loadfile(mapping_t * mapping, FILE * file)
{
  return drake_mapping_loadfilterfile(mapping, file, take_all_tasks);
}

mapping_t*
drake_mapping_loadfilterfile(mapping_t * mapping, FILE * file, int (filter)(task_t*))
{
/*  char line [ 128 ];
  printf("[%s] ", __func__);
        while ( fgets ( line, sizeof line, file ) != NULL )
        {
           fputs ( line, stdout );
        }
        //fclose ( file );
        return NULL;*/
  mapping = pelib_drawgraph2_load(file, mapping, filter);
  //printf("[%s] ", __func__);
  //drake_mapping_display(mapping);
  return mapping;/**/
}

char*
drake_mapping_drawstr(mapping_t * mapping, char * str)
{
  unsigned int i, k, proc_index, task_index;
  size_t length;
  FILE *stream;

  // Opens a stream that points to str
  stream = (FILE*)open_memstream(&str, &length);
  assert(stream != NULL);

  // Warm-up
  fprintf(stream, "k = %u\n", mapping->processor_count);
  fprintf(stream, DRAKE_MAPPING_OUT_WARMUP);
  fprintf(stream, "%-*c", DRAKE_MAPPING_OUT_NODE_COLUMN_WIDTH, ':');
  for (i = 0; i < mapping->processor_count; i++)
    {
      fprintf(stream, "%-*u", DRAKE_MAPPING_OUT_PROC_COLUMN_WIDTH, i + 1);
    }
  fprintf(stream, "%s\n", DRAKE_MAPPING_OUT_AFFECT);

  for (task_index = 0; task_index < mapping->task_count; task_index++)
    {
      proc_index = drake_mapping_find_processor_index(mapping,
          drake_mapping_find_processor(mapping, task_index + 1));
      fprintf(stream, "%-*u", DRAKE_MAPPING_OUT_NODE_COLUMN_WIDTH, task_index
          + 1);

      for (k = 0; k < mapping->processor_count; k++)
        {
          if (k != proc_index)
            {
              fprintf(stream, "%-*u", DRAKE_MAPPING_OUT_PROC_COLUMN_WIDTH, 0);
            }
          else
            {
              fprintf(stream, "%-*u", DRAKE_MAPPING_OUT_PROC_COLUMN_WIDTH, 1);
            }
        }
      fprintf(stream, "\n");
    }
  fprintf(stream, DRAKE_MAPPING_OUT_ENDING);

  // Closes the stream
  fflush(stream);
  fclose(stream);

  return str;
}

task_t*
drake_mapping_find_task(mapping_t* mapping, task_id id)
{
	int target_task_rank, target_proc_rank;
	processor_t *target_proc;

	target_proc_rank = drake_mapping_find_processor(mapping, id);
	if(target_proc_rank >= 0)
	{
		target_proc = mapping->proc[target_proc_rank];
		target_task_rank = drake_processor_find_task(target_proc, id);

		if(target_task_rank >= 0)
		{
			return target_proc->task[target_task_rank];
		}
	}
	
	return NULL;
}

int
drake_mapping_draw(mapping_t * mapping, FILE * file)
{
  char * str = NULL;
  FILE *stream;

  str = drake_mapping_drawstr(mapping, str);
  stream = (FILE*)fmemopen(str, strlen(str) + 1, "r");

  mapping_draw(stream, file, stderr);
  fclose(stream);

  free(str);

  return 1;
}

