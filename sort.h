#ifndef INCLUDES_FUNNELSORT_SORT_H
#define INCLUDES_FUNNELSORT_SORT_H
#include <stddef.h>

typedef int (*cmp_t)(const void *, const void *);
void sort(void *ptr, int nmemb, size_t size, cmp_t cmp);

#endif /* INCLUDES_FUNNELSORT_SORT_H */

//#define DBG_MODE		//Эта няша, чтобы понять, как все работает - остальные режимы надо включить
#define PRINT_MODE	//Эта - чтобы выводились input, output
//#define SHOW_DBG		//Эта - чтобы выводились этапы MERGE, TOP
