#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <task.h>
#include <architecture.h>

int
pelib_printf(task_tp)(task_tp task)
{
  char * str = NULL;
  str = pelib_string(task_tp)(task);
  snekkja_stdout("%s\n", str);

  free(str);

  return 1;
}

int
pelib_printf_detail(task_tp)(task_tp task, int level)
{
	char *str;
	str = pelib_string_detail(task_tp)(task, level);
	printf("%s\n", str);
	free(str);

	return 0;
}

//#define task_str_det_chk printf("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
#define task_str_det_chk
char *
pelib_string(task_tp)(task_tp task)
{
	size_t size;
	char *str;
task_str_det_chk

	if(task == NULL)
	{
task_str_det_chk
		str = malloc(sizeof(char) * 2);
task_str_det_chk
		sprintf(str, ".");
  		return str;
task_str_det_chk
	}

task_str_det_chk
	if(task->id > 0)
	{
task_str_det_chk
		size = (size_t)log10(task->id);
task_str_det_chk
	}
	else
	{
task_str_det_chk
		size = 1;
task_str_det_chk
	}
	
task_str_det_chk
  str = malloc((size + 1) * sizeof(char));
task_str_det_chk
  sprintf(str, "%u", task->id);
task_str_det_chk

  return str;
}

char*
pelib_task_tp_string_detail(task_tp task, int level)
{
task_str_det_chk
	if(level == 0)
	{
task_str_det_chk
		return pelib_string(task_tp)(task);
	}
	else
	{
		char *simple_str, *complete_str, *pred_str, *succ_str, *src_str, *sink_str;

task_str_det_chk
		simple_str = pelib_string(task_tp)(task);
task_str_det_chk
		if(task->pred != NULL)
		{
task_str_det_chk
			pred_str = pelib_string_detail(array_t(link_tp))(*task->pred, level - 1);
task_str_det_chk
		}
		else
		{
task_str_det_chk
			pred_str = malloc(sizeof(char));
task_str_det_chk
			pred_str[0] = '\0';
task_str_det_chk
		}
		
		if(task->pred != NULL)
		{
task_str_det_chk
			succ_str = pelib_string_detail(array_t(link_tp))(*task->succ, level - 1);
task_str_det_chk
		}
		else
		{
			succ_str = malloc(sizeof(char));
			succ_str[0] = '\0';
		}
		
		if(task->source != NULL)
		{
task_str_det_chk
			src_str = pelib_string_detail(array_t(cross_link_tp))(*task->source, level - 1);
task_str_det_chk
		}
		else
		{
			src_str = malloc(sizeof(char));
			src_str[0] = '\0';
		}

		if(task->sink != NULL)
		{
task_str_det_chk
			sink_str = pelib_string_detail(array_t(cross_link_tp))(*task->sink, level - 1);
task_str_det_chk
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
pelib_task_alloc()
{
	return malloc(sizeof(task_t));
}

task_tp*
pelib_task_tp_alloc(void* aux)
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
pelib_fread(task_tp)(task_tp* ptr, size_t size, size_t nmemb, FILE* stream)
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

#define ARRAY_T task_tp
#include <array.c>
