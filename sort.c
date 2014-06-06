#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include "sort.h"
#include <math.h>

//Для вывода шагов на примере маленького массива - вкл. DBG_MODE в sort.h

#ifndef DBG_MODE
#define CACHELINE_SIZE 64
#define BUF_SIZE CACHELINE_SIZE/4
//Размер буфера указан в количестве элементов
#else
#define CACHELINE_SIZE 13
#define BUF_SIZE 3
#endif


int comp(const void *i, const void *j)
{
	return *(int *)i - *(int *)j;
}

struct funnel {
	struct funnel **sub_funnel;	//Подструктуры
	int *own_part_of_array;		//Ссылка на кусок массива под ответственностью funnel
	int n;						//Общее число элементов внутри под ответственностью funnel 
	int groups_amount;			//Число подструктур
	int stream_size;			//Размер потока,
	//т.е. число еще не вытащенных элементов на уровень родителя
	//При извлечении из буфера - уменьшается, изначально = n
	//Если 0 - состояние FUNNEL EXHAUSTED 
	
	int *buf;					//Буфер на вывод
	//BUF_SIZE - глобальная - 	//Максимальный размер буфера
	
	/*Работа с буфером
	* Буфер - на самом деле массив, память под него выделяется однократно,
	* есть указатели на начало и конец, они циклически перемещаются при
	* добавлении/извлечении элемента. Надо следить за тем, чтобы число 
	* элементов было не больше заданного размера.
	* Есть операции: извлечения из head, тихого извлечения из head, записи
	* после tail
	*/
	
	int in_buf;					//Число элементов в буфере на текущий момент
	int *head;
	int *tail;
	//Буфер считается заполненным, если in_buf == BUF_SIZE
	//или если in_buf == stream_size
	//Во втором случае работа с таким дочерним funnel заканчивается
};

void print_funnel(struct funnel* funnel)
{
	//DBG
	int i;
	fprintf(stdout, "PRINT FUNNEL:\n");
	int *arr = funnel->own_part_of_array;
	for (i=0; i<funnel->n; i++)
		fprintf(stdout, "%d ", arr[i]);
	fprintf(stdout, "\nF_ENDS\n");
}

void print_funnel_buf(struct funnel* funnel)
{
	//DBG
	int i;
	fprintf(stdout, "funnel buf with %d el:\n", funnel->in_buf);
	int *arr = funnel->buf;
	for (i=0; i<BUF_SIZE; i++)
		fprintf(stdout, "%d ", arr[i]);
	fprintf(stdout, "\n%p %p %p\n", funnel->buf, funnel-> head, funnel->tail);
	fprintf(stdout, "F_BUF_ENDS\n");
}

struct funnel* funnel_create(int* arr, int n)
{
	//Рекурсивное создание funnel
	struct funnel *funnel = (struct funnel *) malloc(sizeof(struct funnel));
	funnel->n = n;
	funnel->own_part_of_array = arr;
	funnel->groups_amount = 0;	//По этому значению также определяем, лист ли это
	
	funnel->buf = (int*)malloc(BUF_SIZE * sizeof(int));	//Буфер фикс. длины
	funnel->stream_size = n;			//Элементов осталось извлечь из этого потока
	funnel->in_buf = 0;					//Буфер изначально пуст
	funnel->tail = funnel->buf;			//Указатель на первый элемент
	funnel->head = funnel->buf + 1;		//Теперь, если запишем элемент в буфер - 
	// голова и хвост совпадут, указывая на записанный элемент
	
	if (sizeof(int) * n > CACHELINE_SIZE) 
	{
		//Узел
		
		int in_group = (int)floor(pow(n, 2.0/3) + 0.00001);
		funnel->groups_amount = (int)ceil((double)n / in_group);
		int n_right = n - in_group*(funnel->groups_amount-1);
		//в последней группе возможен недобор
		
		fprintf(stdout, "DEL %d on %d with %d\n", n, funnel->groups_amount, in_group);
		
		funnel->sub_funnel = (struct funnel**)malloc(sizeof(struct funnel*) * funnel->groups_amount);
		
		int counter;
		for (counter = 0; counter < funnel->groups_amount-1; counter++)
		{
			funnel->sub_funnel[counter] = 
			funnel_create(funnel->own_part_of_array + in_group*counter, in_group);
		}
		funnel->sub_funnel[counter] = 
		funnel_create(funnel->own_part_of_array + in_group*counter, n_right);
	} 
	else 
	{
		//Это лист - запишем в заведомо вместительный буфер отсортированные
		//элементы
		memcpy(funnel->buf, funnel->own_part_of_array, funnel->n*sizeof(int));
		qsort(funnel->buf, funnel->n, sizeof(int), comp);
		//Указатели надо правильно поставить
		funnel->head = funnel->buf;
		funnel->tail = funnel->buf + (n - 1);
		funnel->in_buf = n;
	}

	return funnel;
}

int buf_pop(struct funnel* funnel)
{
	//Извлечение значения из выходного буфера со сдвигом указателей и
	//прочим весельем
	int ret = funnel->head[0];
	if (funnel->head == funnel->buf + (BUF_SIZE-1))
		funnel->head = funnel->buf;	//Циклический сдвиг
	else funnel->head += 1;
	
	funnel->stream_size -= 1;	//Кто-то сверху будет отвечать за элемент 
	funnel->in_buf -= 1;
	
	return ret;
}

int buf_silent_pop(struct funnel* funnel)
{
	//Посмотрим на головной элемент, без побочных эффектов
	//Если надо будет мержиться элементами, например 
	return funnel->head[0];
}

void buf_push(struct funnel* funnel, int value)
{
	//Сохранение значения в хвосте буфера - после tail
	if (funnel->tail == funnel->buf + (BUF_SIZE-1))
		funnel->tail = funnel->buf;	//Циклический сдвиг
	else funnel->tail += 1;
	funnel->tail[0] = value;									//check
	
	funnel->in_buf += 1;
}

int return_index_of_least(struct funnel* funnel)
{
	//Проход в лоб по списку из адресов heads подгрупп с целью выбора
	//наменьшего, возвращает номер подгруппы, откуда потом вытащим элемент
	//-1, если все подгруппы с пустыми буферами - в теории не произойдет
	int min_index = -1;
	int i;
	for (i=0; i<funnel->groups_amount; i++)
	{
		if (funnel->sub_funnel[i]->in_buf > 0)
		{
			if (min_index == -1)
				min_index = i;
			if (comp(funnel->sub_funnel[i]->head, funnel->sub_funnel[min_index]->head) < 0)
			{
				min_index = i;
			}
		}
	}
	return min_index;
}

void funnel_fill(struct funnel* funnel)
{
	/* FILL(v):
	 * Пока буфер не заполнился
	 * (в том числе, буфер заполнен, когда он содержит столько элементов, 
	 * сколько осталось вытащить из потока)
	 * 		>
	 * 		>	Если буфер какой-то из подгрупп пуст, при этом подгруппа -
	 * 		>	не лист, и из ее потока еще что-то надо извлечь, т.е. -  
	 * 		>	не EXHAUSTED - тогда FILL(sub_v)
	 * 		>	и так будет с каждым!
	 * 		>	
	 * 		>	Шаг мержа (поиск минимального в подбуферах, pop оттуда, push сюда)
	 * 
	*/
	
	if (funnel->groups_amount == 0) {
		//Лист
		//Вроде как по алгоритму здесь невозможно оказаться
		
		//qsort(funnel->in, funnel->nmemb, funnel->size, funnel->cmp);
		//memcpy(funnel->out, funnel->in, funnel->nmemb * funnel->size);
		
		print_funnel(funnel);
		fprintf(stdout, "returning from BOTTOM \n");
		return;
	}
	while (funnel->in_buf < BUF_SIZE && funnel->in_buf < funnel->stream_size)
	{
		int i;
	
		//У кого-то надо будет обновить буфер
		for(i=0; i<funnel->groups_amount; i++)
			if (funnel->sub_funnel[i]->groups_amount > 0 && 
				funnel->sub_funnel[i]->in_buf == 0 &&
				funnel->sub_funnel[i]->stream_size != 0
				)	funnel_fill(funnel->sub_funnel[i]);
		//fprintf(stdout, "BUFS_UPDATED\n");
		
		//Подбуферы обновлены, некоторые пусты - но точно есть хоть один 
		//непустой. Выберем наименьший элемент:
		
		i = return_index_of_least(funnel);
		buf_push(funnel, buf_pop(funnel->sub_funnel[i]));
		fprintf(stdout, "MERGE\n");
	}
}

void sort(int *arr, int n)
{
	//будем складывать данные из буфера главного funnel сюда:
	int *out = (int*)malloc(sizeof(int) * n);
	int ptr = 0;
	
	struct funnel* funnel = funnel_create(arr, n);
	fprintf(stdout, "FUNNEL_CREATED\n");
	
	do{
		fprintf(stdout, "TOP\n");
		funnel_fill(funnel);
		while (funnel->in_buf > 0)
		{
			out[ptr++] = buf_pop(funnel);
		}
	} while (funnel->stream_size > 0);
	
	memcpy(arr, out, n*sizeof(int));
	#ifdef DBG_MODE
	fprintf(stdout, "sorted in DBG_MODE\n");
	#else
	fprintf(stdout, "sorted in WORK_MODE\n");
	#endif
}
