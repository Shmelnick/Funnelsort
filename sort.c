#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "sort.h"
#include <math.h>
//#define CACHELINE_SIZE 64
#define CACHELINE_SIZE 17
#define BUF_SIZE 3

int comp(const void *i, const void *j)
{
	return *(int *)i - *(int *)j;
}

struct funnel {
	struct funnel **sub_funnel;	//Подструктуры
	int *own_part_of_array;	//Ссылка на кусок массива под нашей ответственностью
	int n;	//Общее число элементов внутри  
	int *out;	//Область памяти
	int next_index;	//Следующая заполняемая позиция в Области памяти
	int groups_amount;	//Число подструктур
	
	bool leaf;	//Листовой ли узел	(в том смысле, что он - просто буфер)
	int *buf;	//Буфер на вывод
	//BUF_SIZE - глобальная - Максимальный размер буфера
	int stream_size;	//Размер потока,
	//т.е. число еще не вытащенных элементов на уровень родителя
	//При извлечении из буфера - уменьшается, изначально = n
	//Если 0 - состояние FUNNEL EXHAUSTED 
	
	/*Работа с буфером
	* Буфер - на самом деле массив, память под него выделяется однократно,
	* есть указатели на начало и конец, они циклически перемещаются при
	* добавлении/извлечении элемента. Надо следить за тем, чтобы число 
	* элементов было не больше заданного размера.
	* Есть операции: извлечения из head, тихого извлечения из head, записи
	* после tail
	*/
	
	int in_buf;	//Число элементов в буфере
	int *head;
	int *tail;
	//Буфер считается заполненным, если in_buf == BUF_SIZE
	//или если in_buf == stream_size
	
};

void print_funnel(struct funnel* funnel)
{
	int i;
	fprintf(stdout, "PRINT FUNNEL:\n");
	int *arr = funnel->own_part_of_array;
	for (i=0; i<funnel->n; i++)
		fprintf(stdout, "%d ", arr[i]);
	fprintf(stdout, "\nF_ENDS\n");
}

void print_funnel_out(struct funnel* funnel)
{
	int i;
	fprintf(stdout, "PRINT FUNNEL OUT:\n");
	int *arr = funnel->out;
	for (i=0; i<funnel->n; i++)
		fprintf(stdout, "%d ", arr[i]);
	fprintf(stdout, "\nF_OUT_ENDS\n");
}

struct funnel* funnel_create(int* arr, int n, int* out)
{
	struct funnel *funnel = (struct funnel *) malloc(sizeof(struct funnel));
	funnel->out = out;
	funnel->n = n;
	funnel->own_part_of_array = arr;
	funnel->next_index = 0;
	funnel->groups_amount = 0;
	
	funnel->leaf = true;
	*buf = (int*)malloc(BUF_SIZE * sizeof(int));
	
	//print_funnel(funnel);

	if (sizeof(int) * n > CACHELINE_SIZE) {
		//Узел
		funnel->leaf = false;
		
		int in_group = (int)floor(pow(n, 2.0/3) + 0.00001);
		funnel->groups_amount = (int)ceil((double)n / in_group);
		int *out_for_deep = (int*)malloc(n * sizeof(int));
		int n_right = n - in_group*(funnel->groups_amount-1);
		
		fprintf(stdout, "DEL %d on %d with %d\n", n, funnel->groups_amount, in_group);
		
		funnel->sub_funnel = (struct funnel**)malloc(sizeof(struct funnel*) * funnel->groups_amount);
		
		int counter;
		for (counter = 0; counter < funnel->groups_amount-1; counter++)
		{
			funnel->sub_funnel[counter] = 
funnel_create(	funnel->own_part_of_array + in_group*counter,	// * sizeof(int)
				in_group, 
				out_for_deep + in_group*counter);	// * sizeof(int)
		}
		funnel->sub_funnel[counter] = 
	funnel_create(	funnel->own_part_of_array + in_group*counter, 
					n_right,
					out_for_deep + in_group*counter);
	} else {
		//groups_amount is still =0
		;
	}

	return funnel;
}

int* funnel_pop(struct funnel* funnel)
{
	if (funnel->next_index >= funnel->n)
		return NULL;
	return funnel->out + funnel->next_index++;	// * funnel->size
}

int return_index_of_less(int** array_of_addresses, int len)
{
	int min_index = -1;
	int i;
	for (i=0; i<len; i++)
	{
		if (array_of_addresses[i] != NULL)
		{
			if (min_index == -1)
				min_index = i;
			if (comp(array_of_addresses[i], array_of_addresses[min_index]) < 0)
			{
				min_index = i;
			}
		}
	}
	return min_index;
}

void print_indexes_array(int** array_of_addresses, int len)
{
	int i;
	fprintf(stdout, "Indexes: ");
	for (i=0; i<len; i++)
		fprintf(stdout, "%p ", array_of_addresses[i]);
	fprintf(stdout, "\n");
}

void funnel_fill(struct funnel* funnel)
{
	if (funnel->groups_amount == 0) {
		//qsort(funnel->in, funnel->nmemb, funnel->size, funnel->cmp);
		//memcpy(funnel->out, funnel->in, funnel->nmemb * funnel->size);
		
		//Копируем в память, делаем там простую сортировку
		memcpy(funnel->out, funnel->own_part_of_array, funnel->n*sizeof(int));
		qsort(funnel->out, funnel->n, sizeof(int), comp);
		
		print_funnel_out(funnel);
		fprintf(stdout, "returning from BOTTOM \n");
		return;
	}
	//Иначе сортируем детей и сливаем в funnel->out
	int i;
	
	for(i=0; i<funnel->groups_amount; i++)
		funnel_fill(funnel->sub_funnel[i]);
	
	int** mini = (int**)malloc(sizeof(int*) * funnel->groups_amount);
	for(i=0; i<funnel->groups_amount; i++)
		mini[i] = funnel_pop(funnel->sub_funnel[i]);
	
	int pos_in_out = 0;	
	i = return_index_of_less(mini, funnel->groups_amount);
	//fprintf(stdout, "<< %d \n", funnel->sub_funnel[0]->out[4]);		
	
	//print_funnel_out(funnel);
	print_indexes_array(mini, funnel->groups_amount);
	
	while (i != -1)
	{
		memcpy(funnel->out + pos_in_out++, mini[i], sizeof(int));	//*
		mini[i] = funnel_pop(funnel->sub_funnel[i]);
		i = return_index_of_less(mini, funnel->groups_amount);
		print_indexes_array(mini, funnel->groups_amount);
	}
	
	fprintf(stdout, "MERGE\n");
}

void sort(int *arr, int n)
{
	int size = sizeof(int);
	int *out = (int*)malloc(sizeof(int) * n);
	
	fprintf(stdout, "INT:%d\n", size);
	struct funnel* funnel = funnel_create(arr, n, out);
	fprintf(stdout, "FUNNEL_CREATED\n");
	funnel_fill(funnel);
	memcpy(arr, out, sizeof(int) * n);
	print_funnel_out(funnel);
}
