#include <mapping.h>
#include <architecture.h>
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
      mapping->processor_count = 0;
      mapping->max_processor_count = size;
      mapping->task_count = 0;

      // Makes sure the allocated memory was actually allocated
      if (res != 0)
        {
          return mapping;
        }
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

  free(solution);

  return 1;
}

int
pelib_copy(mapping_t)(mapping_t source, mapping_t *copy)
{
  size_t i;

  //copy = pelib_alloc(mapping_t)(PELIB_MAPPING_MAX_TASK_COUNT);
  copy->processor_count = source.processor_count;
  copy->task_count = source.task_count;

  for (i = 0; i < copy->processor_count; i++)
    {
      //pelib_processor_destroy(copy->proc[i]);
      pelib_copy(processor_t)(*source.proc[i], copy->proc[i]);
    }

  return 1;
}

int
pelib_mapping_insert_task(mapping_t* mapping, processor_id pid, task_t* task)
{
  int processor_index;

  processor_index = pelib_mapping_find_processor_index(mapping, pid);
  if (pelib_processor_insert_task(mapping->proc[processor_index], task)
      == 1)
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
pelib_mapping_remove_task(mapping_t* mapping, task_id task)
{
  processor_id pid;

  pid = pelib_mapping_find_processor(mapping, task);

  if (pid < mapping->processor_count && pelib_processor_remove_task(
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
pelib_mapping_find_processor_index(mapping_t* mapping, processor_id pid)
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
pelib_mapping_find_processor(mapping_t* mapping, task_id tid)
{
  int i;
  for (i = 0; i < mapping->processor_count; i++)
    {
      if (pelib_processor_find_task(mapping->proc[i], tid)
          != mapping->proc[i]->handled_nodes)
        {
          return mapping->proc[i]->id;
        }
    }

  return -1;
}
int
pelib_mapping_insert_processor(mapping_t* mapping, processor_t* processor)
{
  // If there is room for a new processor in mapping, and this processor id was not already present
  if (mapping->processor_count < mapping->max_processor_count
      && pelib_mapping_find_processor_index(mapping, processor->id)
          == mapping->processor_count)
    {
      pelib_copy(processor_t)(*processor, mapping->proc[mapping->processor_count]);

//      printf("%s: %X\n", __FUNCTION__, mapping->proc[mapping->processor_count]);

      if (mapping->proc[mapping->processor_count] != NULL)
        {
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
pelib_mapping_remove_processor(mapping_t* mapping, int processor_id)
{
  // TODO: implement
  return 0;
}

// Mapping properties
int
pelib_mapping_violations(mapping_t* mapping)
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
          found_pid = pelib_mapping_find_processor(mapping,
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
int
pelib_printf(mapping_t)(mapping_t mapping)
{
  char *str = pelib_string(mapping_t)(mapping);
  snekkja_stdout("%s\n", str);

  free(str);

  return 0;
}
char*
pelib_string(mapping_t)(mapping_t mapping)
{
  size_t i, total_length;
  char *proc_str, *str;

  total_length = 0;
  for (i = 0; i < mapping.processor_count; i++)
    {
      total_length += (PELIB_MAPPING_PROC_ID_CHAR_LENGTH
          + (sizeof(PELIB_MAPPING_SEPARATOR) + PELIB_MAPPING_NODE_CHAR_LENGTH)
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
  stream_str = fmemopen((void*)str, strlen(str), "r");
 // printf("file descriptor: %i\n", *stream_str);

  mapping = pelib_mapping_loadfilterfile(mapping, stream_str, filter);
  fclose(stream_str);
  //printf("%i\n", i == EOF);

  return mapping;
}

mapping_t*
pelib_mapping_loadstr(mapping_t * mapping, char * str)
{
  return mapping_loadstr(mapping, str, take_all_tasks);
}

mapping_t*
pelib_mapping_loadfilterstr(mapping_t* mapping, char* str, int (filter)(task_t*))
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
pelib_mapping_loadfile(mapping_t * mapping, FILE * file)
{
  return pelib_mapping_loadfilterfile(mapping, file, take_all_tasks);
}

mapping_t*
pelib_mapping_loadfilterfile(mapping_t * mapping, FILE * file, int (filter)(task_t*))
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
  //pelib_mapping_display(mapping);
  return mapping;/**/
}

char*
pelib_mapping_drawstr(mapping_t * mapping, char * str)
{
  unsigned int i, k, proc_index, task_index;
  unsigned int length;
  FILE *stream;

  // Opens a stream that points to str
  stream = open_memstream(&str, &length);
  assert(stream != NULL);

  // Warm-up
  fprintf(stream, "k = %u\n", mapping->processor_count);
  fprintf(stream, PELIB_MAPPING_OUT_WARMUP);
  fprintf(stream, "%-*c", PELIB_MAPPING_OUT_NODE_COLUMN_WIDTH, ':');
  for (i = 0; i < mapping->processor_count; i++)
    {
      fprintf(stream, "%-*u", PELIB_MAPPING_OUT_PROC_COLUMN_WIDTH, i + 1);
    }
  fprintf(stream, "%s\n", PELIB_MAPPING_OUT_AFFECT);

  for (task_index = 0; task_index < mapping->task_count; task_index++)
    {
      proc_index = pelib_mapping_find_processor_index(mapping,
          pelib_mapping_find_processor(mapping, task_index + 1));
      fprintf(stream, "%-*u", PELIB_MAPPING_OUT_NODE_COLUMN_WIDTH, task_index
          + 1);

      for (k = 0; k < mapping->processor_count; k++)
        {
          if (k != proc_index)
            {
              fprintf(stream, "%-*u", PELIB_MAPPING_OUT_PROC_COLUMN_WIDTH, 0);
            }
          else
            {
              fprintf(stream, "%-*u", PELIB_MAPPING_OUT_PROC_COLUMN_WIDTH, 1);
            }
        }
      fprintf(stream, "\n");
    }
  fprintf(stream, PELIB_MAPPING_OUT_ENDING);

  // Closes the stream
  fflush(stream);
  fclose(stream);

  return str;
}

task_t*
pelib_mapping_find_task(mapping_t* mapping, task_id id)
{
	int target_task_rank, target_proc_rank;
	processor_t *target_proc;

	target_proc_rank = pelib_mapping_find_processor(mapping, id);
	if(target_proc_rank >= 0)
	{
		target_proc = mapping->proc[target_proc_rank];
		target_task_rank = pelib_processor_find_task(target_proc, id);

		if(target_task_rank >= 0)
		{
			return target_proc->task[target_task_rank];
		}
	}
	
	return NULL;
}

int
pelib_mapping_draw(mapping_t * mapping, FILE * file)
{
  char * str = NULL;
  FILE *stream;

  str = pelib_mapping_drawstr(mapping, str);
  stream = fmemopen(str, strlen(str) + 1, "r");

  mapping_draw(stream, file, stderr);
  fclose(stream);

  free(str);

  return 1;
}

