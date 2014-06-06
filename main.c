#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "sort.h"

#ifdef DBG_MODE
enum {SIZE = 9} ;
#else
enum {SIZE = 4096} ;
#endif

int data[SIZE];

void
generate(int *data, int size)
{
	srand((unsigned) time(0));
	for (int i = 0; i < size; i++)
	{
		//data[i] = rand() % 10 + 1;
		data[i] = rand();
	}
}

int
intcmp(const void *p_a, const void *p_b)
{
	return *(int *) p_a -  *(int *) p_b;
}

void
output(int *data, int size)
{
	for (int i = 0; i < size; i++)
		printf("%d\n", data[i]);
}

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
	generate(data, SIZE);
	
	output(data, SIZE);
	//sort(data, SIZE, sizeof(*data), intcmp);
	sort(data, SIZE);
	#ifdef PRINT_MODE
	output(data, SIZE);
	fprintf(stdout, "PRINT_MODE\n");
	#else
	fprintf(stdout, "PRINT_RES_OFF\n");
	#endif

	return 0;
}
