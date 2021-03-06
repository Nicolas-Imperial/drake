#if 0
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
#include <drake/cross_link.h>
#include <drake/platform.h>

cross_link_tp *
pelib_alloc_struct(cross_link_tp)()
{
	return malloc(sizeof(cross_link_tp));
}

cross_link_tp *
pelib_alloc(cross_link_tp)(void* aux)
{
	return pelib_alloc_struct(cross_link_tp)();
}

int
pelib_init(cross_link_tp)(cross_link_tp *link)
{
	*link = NULL;
	return 1;
}

int
pelib_copy(cross_link_tp)(cross_link_tp src, cross_link_tp* dst)
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
pelib_free_struct(cross_link_tp)(cross_link_tp * link)
{
	free(link);

	return 1;
}

int
pelib_free(cross_link_tp)(cross_link_tp *link)
{
	return pelib_free_struct(cross_link_tp)(link);
}

int
pelib_compare(cross_link_tp)(cross_link_tp l1, cross_link_tp l2)
{
	return 0;
}

FILE*
pelib_printf(cross_link_tp)(FILE* stream, cross_link_tp link)
{
	return pelib_printf_detail(cross_link_tp)(stream, link, 0);
}

FILE*
pelib_printf_detail(cross_link_tp)(FILE* stream, cross_link_tp link, int level)
{
	char* str;

	str = pelib_string_detail(cross_link_tp)(link, level);
	printf("%s", str);
	free(str);

	return stream;
}

size_t
pelib_fwrite(cross_link_tp)(cross_link_tp link, FILE* file)
{
	fprintf(stderr, "[%s:%d] Not implemented\n", __FILE__, __LINE__);
	return 0;
}

size_t
pelib_fread(cross_link_tp)(cross_link_tp* link, FILE* file)
{
	fprintf(stderr, "[%s:%d] Not implemented\n", __FILE__, __LINE__);
	return 0;
}

char*
pelib_string(cross_link_tp)(cross_link_tp link)
{
	return pelib_string_detail(cross_link_tp)(link, 0);
}

char*
pelib_string_detail(cross_link_tp)(cross_link_tp link, int level)
{
	char *str, *link_str;

	if(level == 0)
	{
		link_str = pelib_string(link_tp)(link->link);

		return link_str;
	}
	else
	{
		link_str = pelib_string_detail(link_tp)(link->link, level - 1);
		str = malloc(sizeof(char) * (strlen(link_str) + 3 + sizeof(cross_link_tp) * 2 + 1 + 1 + 1));

		sprintf(str, "->{%p:%s}", link, link_str);
		free(link_str);

		return str;
	}
}

#define ARRAY_T cross_link_tp
#include <pelib/array.c>
#endif
