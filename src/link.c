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

#include <drake/link.h>
#include <drake/platform.h>

link_tp *
pelib_alloc_struct(link_tp)()
{
	return malloc(sizeof(link_tp));
}

link_tp*
pelib_alloc(link_tp)(void* aux)
{
	return pelib_alloc_struct(link_tp)();
}

int
pelib_init(link_tp)(link_tp* link)
{
	*link = NULL;

	return 1;
}

int
pelib_free_struct(link_tp)(link_tp* link)
{
	free(link);

	return 1;
}

int
pelib_free(link_tp)(link_tp* link)
{
	return pelib_free(link_tp)(link);
}

int
pelib_copy(link_tp)(link_tp src, link_tp* dst)
{
	if(dst != NULL)
	{
		*dst = src;
		return 1;
	}
	else
	{
		return 0;
	}
}

int
pelib_compare(link_tp)(link_tp l1, link_tp l2)
{
	return 0;
}

FILE*
pelib_printf(link_tp)(FILE* stream, link_tp link)
{
	char *str;
	str = pelib_string(link_tp)(link);
	fprintf(stream, "%s\n", str);

	free(str);

	return stream;
}

FILE*
pelib_printf_detail(link_tp)(FILE* stream, link_tp link, int level)
{
	char *str;
	str = pelib_string_detail(link_tp)(link, level);
	fprintf(stream, "%s\n", str);

	free(str);

	return stream;
}

size_t
pelib_fwrite(link_tp)(link_tp link, FILE* file)
{
	fprintf(stderr, "[%s:%d] Not implemented\n", __FILE__, __LINE__);
	return 0;
}

size_t
pelib_fread(link_tp)(link_tp* link, FILE* file)
{
	fprintf(stderr, "[%s:%d] Not implemented\n", __FILE__, __LINE__);
	return 0;
}

//#define link_str_chk printf("[%s:%s:%d] Hello world!\n", __FILE__, __FUNCTION__, __LINE__);
#define link_str_chk
char*
pelib_string(link_tp)(link_tp link)
{
link_str_chk
	return pelib_string_detail(link_tp)(link, 0);
}

char*
pelib_string_detail(link_tp)(link_tp link, int level)
{
	char *task, *str, *cfifo;

link_str_chk
	if(link->cons != NULL)
	{
link_str_chk
		task = pelib_string_detail(task_tp)(link->cons, level == 0 ? level : level - 1);
link_str_chk
	}
	else
	{
link_str_chk
		task = malloc(strlen(".") + 1);
link_str_chk
		sprintf(task, "%s", ".");
link_str_chk
	}

link_str_chk
	if(level == 0)
	{
link_str_chk
		str = malloc(strlen(task) + 3);
link_str_chk
		sprintf(str, "->%s", task);
link_str_chk
	}
	else
	{
link_str_chk
		if(link->buffer != NULL)
		{
link_str_chk
//			cfifo = pelib_string_detail(cfifo_t(int))(*link->buffer, level - 1);
			cfifo=malloc(sizeof(char) * sizeof(link_tp) * 2);
			sprintf(cfifo, "%p", link->buffer);
link_str_chk
		}
		else
		{
link_str_chk
			cfifo = malloc(sizeof(char) * 2);
link_str_chk
			sprintf(cfifo, ".");
link_str_chk
		}

link_str_chk
		str = malloc(strlen(cfifo) + strlen(task) + 3 + sizeof(link_tp) * 2 + 2 + 2 + 1 + 1);
link_str_chk
		sprintf(str, "->{%p:%s->%s}", link, cfifo, task);
link_str_chk

		free(cfifo);
link_str_chk
	}

link_str_chk
	free(task);

link_str_chk
	return str;
}

int
pelib_destroy(link_tp)(link_tp link)
{
	return 1;
}

#define PAIR_KEY_T MAP_KEY_TYPE
#define PAIR_VALUE_T link_tp
#include <pelib/pair.c>

#define ITERATOR_T pair_t(MAP_KEY_TYPE, link_tp)
#include <pelib/iterator.c>

#define MAP_KEY_T MAP_KEY_TYPE
#define MAP_VALUE_T link_tp
#include <pelib/map.c>

#define ARRAY_T link_tp
#include <pelib/array.c>
