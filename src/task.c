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
#include <math.h>

#include <drake/link.h>
#include <drake/task.h>
#include <drake/platform.h>

FILE*
pelib_printf(task_tp)(FILE* stream, task_tp task)
{
  char * str = NULL;
  str = pelib_string(task_tp)(task);
  fprintf(stream, "%s", str);

  free(str);

  return stream;
}

FILE*
pelib_printf_detail(task_tp)(FILE* stream, task_tp task, int level)
{
	char *str;
	str = pelib_string_detail(task_tp)(task, level);
	fprintf(stream, "%s", str);
	free(str);

	return stream;
}

char *
pelib_string(task_tp)(task_tp task)
{
	size_t size;
	char *str;

	if(task == NULL)
	{
		str = malloc(sizeof(char) * 2);
		sprintf(str, ".");
  		return str;
	}

	if(task->id > 0)
	{
		size = (size_t)log10(task->id);
	}
	else
	{
		size = 1;
	}
	
  str = malloc((size + 1) * sizeof(char));
  sprintf(str, "%u", task->id);

  return str;
}

char*
pelib_string_detail(task_tp)(task_tp task, int level)
{
	if(level == 0)
	{
		return pelib_string(task_tp)(task);
	}
	else
	{
		char *simple_str, *complete_str, *pred_str, *succ_str, *src_str, *sink_str;

		simple_str = pelib_string(task_tp)(task);
		if(task->pred != NULL)
		{
			pred_str = pelib_string_detail(array_t(link_tp))(*task->pred, level - 1);
		}
		else
		{
			pred_str = malloc(sizeof(char));
			pred_str[0] = '\0';
		}
		
		if(task->pred != NULL)
		{
			succ_str = pelib_string_detail(array_t(link_tp))(*task->succ, level - 1);
		}
		else
		{
			succ_str = malloc(sizeof(char));
			succ_str[0] = '\0';
		}
		
		if(task->source != NULL)
		{
			src_str = pelib_string_detail(array_t(cross_link_tp))(*task->source, level - 1);
		}
		else
		{
			src_str = malloc(sizeof(char));
			src_str[0] = '\0';
		}

		if(task->sink != NULL)
		{
			sink_str = pelib_string_detail(array_t(cross_link_tp))(*task->sink, level - 1);
		}
		else
		{
			sink_str = malloc(sizeof(char));
			sink_str[0] = '\0';
		}
		complete_str = malloc((1 + strlen(simple_str) + 6 + strlen(pred_str) + 6 + strlen(succ_str) + 8 + strlen(src_str) + 6 + strlen(sink_str) + 2) * sizeof(char));
		sprintf(complete_str, "{%s:pred=%s:succ=%s:source=%s:sink=%s}", simple_str, pred_str, succ_str, src_str, sink_str);

		free(simple_str);
		free(pred_str);
		free(succ_str);
		free(src_str);
		free(sink_str);

		return complete_str;
	}
}

int
pelib_compare(task_tp)(task_tp a, task_tp b)
{
  return 0;
}

task_t*
pelib_alloc(task)()
{
	return malloc(sizeof(task_t));
}

task_tp*
pelib_alloc(task_tp)(void* aux)
{
	task_tp* res = malloc(sizeof(task_tp));

	return res;
}

int
pelib_init(task_tp)(task_tp* task)
{
	return 0;
}

size_t
pelib_fread(task_tp)(task_tp* ptr, FILE* stream)
{
	return 0;
}

int
pelib_copy(task_tp)(task_tp source, task_tp* dest)
{
/*	task_t src, *dst;
	src = *source;
	dst = *dest;

	dst->pred = pelib_array_task_tp_t_alloc(pelib_array_task_tp_capacity(src.pred));
	dst->succ = pelib_array_task_tp_t_alloc(pelib_array_task_tp_capacity(src.succ));
	pelib_array_task_tp_t_copy(*src.pred, dst->pred);
	pelib_array_task_tp_t_copy(*src.succ, dst->succ);
*/
	*dest = source;
//	printf("%s: task %d, src->pred=%X, dst->pred=%X\n", __FUNCTION__, source->id, source->pred, (*dest)->pred);
	
	return 0;
}

int
drake_task_depleted(task_tp task)
{
	size_t i;
	int all_killed = 1;
	int all_empty = 1;

	if(task == NULL)
	{
		return 1;
	}

	for(i = 0; i < pelib_array_length(link_tp)(task->pred); i++)
	{
		link_tp link = pelib_array_read(link_tp)(task->pred, i);
		all_killed = all_killed && ((link->prod == NULL) ? 1 : link->prod->status >= TASK_KILLED);
		all_empty = all_empty && (pelib_cfifo_length(int)(link->buffer) == 0);
	}

	return all_killed && all_empty;
}

#define ARRAY_T task_tp
#include <pelib/array.c>
