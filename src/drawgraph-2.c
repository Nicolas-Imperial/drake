// drawgraph.c JK May 23, 2008

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <drake/mapping.h>
#include <drake/platform.h>
#include "drawgraph-2.h"

FILE *input, *output, *error;

// left upper corner of image
double xref = 0.0;
double yref = 0.0;

// radius of a circle
double yrad = 225.0;

// distances between nodes
double xdiff;
double ydiff;

// max no of proc and nodes +1
// x[i][j] == 1 iff node i is mapped onto proc j
int x[NMAX][PMAX];
int num_procs = 0;
int num_tasks = 0;

typedef struct prel
{
	int proc;
	int index;
} Prel;

// order in which processors are drawn
Prel ranklist[PMAX];

// actual no of processors and nodes 1..p 1..n
int p, n, logn;

typedef struct pt
{
	double x;
	double y;
} Pt;

//
Pt pos[NMAX];

// routines to draw

void
initimage(void)
{
	fprintf(
	    output,
	    "\
#FIG 3.2\n\
Portrait\n\
Center\n\
Metric\n\
A4 \n\
100.0\n\
Single\n\
-2\n\
1200 2\n");
}

void
dashline(double x1, double y1, double x2, double y2)
{
	fprintf(output, "2 1 1 4 0 0 100 1 -1  6.40 0 0 12 0 0 2\n");
	fprintf(output, "       %d %d %d %d\n", (int) x1, (int) y1, (int) x2,
	    (int) y2);
}

void
circle(double xcenter, double ycenter, double radius)
{
	//  fprintf(output, "circle %lf %lf %lf\n",xcenter,ycenter,radius);
	fprintf(output, "1 1 0 2 0 0 100 1 -1  6.40 1 0.0 %d %d %d %d %d %d %d %d\n",

	(int) xcenter, (int) ycenter, (int) radius, (int) radius, (int) (xcenter
	    - radius), (int) (ycenter - radius), (int) (xcenter + radius),
	    (int) (ycenter + radius));
}

void
arc(double x1, double y1, double x2, double y2)
{
	//  fprintf(output, "arc %lf %lf %lf %lf\n",x1,y1,x2,y2);
	fprintf(output, "2 1 0 2 0 0 100 1 -1  6.40 0 0 12 1 0 2\n");
	fprintf(output, "  2 1 1.0 144.00 200.00\n");
	fprintf(output, "       %d %d %d %d\n", (int) x1, (int) y1, (int) x2,
	    (int) y2);

}

void
textout(double x1, double y1, char* str, int size)
{
	//  fprintf(output, "text %lf %lf %s\n",x1,y1,str);
	fprintf(output, "4 1 0 100 1 0 %d.0 0.0 4 267.0 %lf %d %d %s\\001\n", size,
	    (double) strlen(str) * 100.0, (int) x1, (int) (y1 + yrad / 2.0), str);

}

#define STRL 255

int
inputstructure(void)
{
	char instr[STRL], *endstr;
	int flag = 0;
	int i, j;
	int tmp;
	char str[2], *end;

	while (!feof(input))
	{
		fgets(instr, STRL, input);

		//            fprintf(output, "XX%sXX\n",instr);

		if (!strncmp(instr, "k =", 3))
		{
			flag = 1;
			sscanf(instr + 4, "%d", &p);
			break;
		}
	}
	if (flag == 0)
	{
		fprintf(output, "could not find line beginning with k =\n");
		return 0;
	}
	n = (1 << p) - 1;
	logn = p;

	//printf("YY p=%d, n=%d, logn=%d\n",p,n,logn);

	flag = 0;
	while (!feof(input))
	{
		fgets(instr, STRL, input);
		if (!strncmp(instr, "x [", 3))
		{
			flag = 1;
			break;
		}
	}
	if (flag == 0)
	{
		fprintf(output, "could not find line beginning with x [\n");
		return 0;
	}

	fgets(instr, STRL, input);

	str[1] = '\0';
	i = 1;
	//for (i = 1; i <= n; i++)
	//num_tasks = 1;
	while (1)
	{
		flag = fscanf(input, "%s", (char*) &instr);
		tmp = strtol(instr, &endstr, 10);
		//printf("%i -> %i (endstr = %i, strlen(instr) = %i, flag = %i)\n", i, tmp, endstr - instr, strlen(instr), flag);
		/**/

		//printf("'%s', j = %i, end = %X, end of instr = %X\n", instr, j, end,  instr + strlen(instr));

		/*
		 fscanf(input, "%d", &tmp);
		 if (tmp != i)
		 fprintf(error, "Error: index mismatch\n");
		 //printf("Error: index mismatch\n");
		 */

		j = 1;
		fscanf(input, "%c", &str[0]);
		while (str[0] != '\n')
		{
			tmp = strtol(str, &end, 2);
			if (end - str > 0)
			{
				x[i][j] = tmp;
				num_procs = j;
				j++;
			}
			fscanf(input, "%c", str);
		}

		if (flag == 0 || flag == EOF || endstr != instr + strlen(instr))
		{
			break;
		}
		i++;
		num_tasks = i;
		/**/
	}

	return 1;
}

int
prelcmp(const void *p1, const void *p2)
{
	int index1, index2;

	index1 = ((Prel*) p1)->index;
	index2 = ((Prel*) p2)->index;

	if (index1 < index2)
		return 1;
	else if (index1 > index2)
		return -1;

	return 0;
}

void
rankprocessors(void)
{
	int i, j;

	// fill ranklist first with minimum numbers of nodes placed
	for (j = 1; j <= p; j++)
	{
		ranklist[j].index = n + 1; // no element placed
		ranklist[j].proc = j; // proc j
		for (i = 1; i <= n; i++)
		{
			if (x[i][j] == 1)
			{
				ranklist[j].index = i;
				break;
			}
		}
	}
	qsort((void*) (&(ranklist[1])), p, sizeof(Prel), prelcmp);
}

int
drawproc(int proc, int line)
{
	int i, level;
	int incline;
	int flag;
	char str[100];

	incline = 0;
	for (level = logn - 1; level >= 0; level--)
	{
		flag = 0;
		for (i = 1 << level; i < (1 << (level + 1)); i++)
		{
			if (x[i][proc] == 1)
			{
				flag = 1; // node in this level placed
				pos[i].x = xref + ((double) ((i - (1 << level)) * (1 << (logn - level))
				    + (1 << (logn - 1 - level))) + 0.5) * xdiff;
				pos[i].y = yref + ((double) line + 0.5) * ydiff;
				circle(pos[i].x, pos[i].y, yrad);
				sprintf(str, "%d", i);
				textout(pos[i].x, pos[i].y, str, 20);
			}
		}
		if (flag)
		{
			incline++;
			line++;
		}
	}

	if (incline == 0)
		incline++;

	sprintf(str, "SPE%d", proc);
	textout(480.0, yref + ((double) line - 0.25) * ydiff, str, 24);

	return incline;
}

void
drawline(int line)
{
	dashline(xref, yref + ((double) line) * ydiff, xref + xdiff
	    * (double) (n + 1), yref + ((double) line) * ydiff);
}

void
drawedges(void)
{
	int i;

	for (i = n; i >= 2; i--)
	{
		if (pos[i].y < pos[i / 2].y) // normal case, edges going from top to bottom
			// leaves node at bottom, hits node at top
			arc(pos[i].x, pos[i].y + yrad, pos[i / 2].x, pos[i / 2].y - yrad);
		else
		{ // leaves and hits nodes at appropriate sides
			if (pos[i].x <= pos[i / 2].x) // edge going left to right
				arc(pos[i].x + yrad, pos[i].y, pos[i / 2].x - yrad, pos[i / 2].y);
			else
				// edge going right to left
				arc(pos[i].x, pos[i].y - yrad, pos[i / 2].x + yrad, pos[i / 2].y);
		}
	}
}

void
drawallprocessors(void)
{
	int j;
	int line = 0;

	initimage();
	for (j = 1; j <= p; j++)
	{
		// draw proc ranklist[j].proc starting from line line
		line += drawproc(ranklist[j].proc, line);
		// if j!=p draw horizontal dashline
		if (j < p)
			drawline(line);
	}
	drawedges();
}

void
pelib_drawgraph2_draw(FILE * read_from, FILE * print_to, FILE * err_to)
{
	input = read_from;
	output = print_to;
	error = err_to;

	if (input == NULL)
	{
		input = stdin;
	}
	if (output == NULL)
	{
		output = stdout;
	}
	if (error == NULL)
	{
		error = stderr;
	}

	xdiff = 1.1 * yrad;
	ydiff = 4 * yrad;

	if (inputstructure())
	{
		rankprocessors();
		drawallprocessors();
	}
}

mapping_t*
pelib_drawgraph2_load(FILE* read_from, mapping_t* mapping, int
( filter)(task_t*))
{
	int i, j;
	processor_t *processor = NULL;
	task_t task;

	input = read_from;
	output = fopen("std-bitbucket.txt", "w");
	error = fopen("err-bitbucket.txt", "w");
	//  output = fopen("/dev/null", "w");
	//  error = fopen("/dev/null", "w");
	num_procs = 0;
	num_tasks = 0;
	inputstructure();

	fprintf(stderr, "[%s:%s:%d] mapping = pelib_alloc_collection(mapping_t)(%d);\n", __FILE__, __FUNCTION__, __LINE__, DRAKE_MAPPING_MAX_PROCESSOR_COUNT);
	mapping = pelib_alloc_collection(mapping_t)(DRAKE_MAPPING_MAX_PROCESSOR_COUNT);
	//  mapping->processor_count = p;

	// Fills mapping with processors and processors with tasks
	for (j = 1; j <= num_procs; j++)
	{
		fprintf(stderr, "[%s:%s:%d] processor = pelib_alloc_collection(processor_t)(%d);\n", __FILE__, __FUNCTION__, __LINE__, DRAKE_MAPPING_MAX_TASK_COUNT);
		processor = pelib_alloc_collection(processor_t)(DRAKE_MAPPING_MAX_TASK_COUNT);
		fprintf(stderr, "[%s:%s:%d] processor->id = %d - 1;\n", __FILE__, __FUNCTION__, __LINE__, j);
		processor->id = j - 1;
		fprintf(stderr, "[%s:%s:%d] processor->source = pelib_alloc_collection(array_t(cross_link_tp))(%d);\n", __FILE__, __FUNCTION__, __LINE__, num_tasks);
		processor->source = pelib_alloc_collection(array_t(cross_link_tp))(num_tasks);
//		pelib_init(array_t(task_tp))(processor->source);
		fprintf(stderr, "[%s:%s:%d] processor->sink = pelib_alloc_collection(array_t(cross_link_tp))(%d);\n", __FILE__, __FUNCTION__, __LINE__, num_tasks);
		processor->sink = pelib_alloc_collection(array_t(cross_link_tp))(num_tasks);
//		pelib_init(array_t(task_tp))(processor->sink);
		fprintf(stderr, "[%s:%s:%d] nb_mapping_insert_processor(mapping, processor);\n", __FILE__, __FUNCTION__, __LINE__);
		drake_mapping_insert_processor(mapping, processor);

		for (i = 1; i < num_tasks; i++)
		{
			if (x[i][j] == 1)
			{
				task.id = i;
				if (filter(&task) != 0)
				{
					fprintf(stderr, "[%s:%s:%d] task.id = %d\n", __FILE__, __FUNCTION__, __LINE__, i);

					fprintf(stderr, "[%s:%s:%d] task.pred = pelib_alloc_collection(array_t(link_tp))(%d);\n", __FILE__, __FUNCTION__, __LINE__, num_tasks);
					task.pred = pelib_alloc_collection(array_t(link_tp))(num_tasks);
	//				pelib_init(array_t(link_tp))(task.pred);
					fprintf(stderr, "[%s:%s:%d] task.succ = pelib_alloc_collection(array_t(link_tp))(%d);\n", __FILE__, __FUNCTION__, __LINE__, num_tasks);
					task.succ = pelib_alloc_collection(array_t(link_tp))(num_tasks);
	//				pelib_init(array_t(link_tp))(task.succ);
					fprintf(stderr, "[%s:%s:%d] task.source = pelib_alloc_collection(array_t(cross_link_tp))(%d);\n", __FILE__, __FUNCTION__, __LINE__, num_tasks);
					task.source = pelib_alloc_collection(array_t(cross_link_tp))(num_tasks);
	//				pelib_init(array_t(cross_link_tp))(task.source);
					fprintf(stderr, "[%s:%s:%d] task.sink = pelib_alloc_collection(array_t(cross_link_tp))(%d);\n", __FILE__, __FUNCTION__, __LINE__, num_tasks);
					task.sink = pelib_alloc_collection(array_t(cross_link_tp))(num_tasks);
	//				pelib_init(array_t(cross_link_tp))(task.sink);

					fprintf(stderr, "[%s:%s:%d] task.status = TASK_INIT;\n", __FILE__, __FUNCTION__, __LINE__);
					task.status = TASK_INIT;

					fprintf(stderr, "[%s:%s:%d] drake_mapping_insert_task(mapping, %d - 1, &task);\n", __FILE__, __FUNCTION__, __LINE__, j);
					drake_mapping_insert_task(mapping, j - 1, &task);
				}
				continue;
			}
		}
	}

	fflush(stdin);
	fflush(stdout);
	fflush(stderr);

	input = stdin;
	output = stdout;
	error = stderr;

	return mapping;
}

