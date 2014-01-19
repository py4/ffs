#include <stdio.h>
#include <stdlib.h>
#include "vector.h"

void vector_init(vector* v,int unit_size)
{
	v->data = NULL;
	v->unit_size = unit_size;
	v->size = 0;
	v->count = 0;
}

int vector_count(vector* v)
{
	return v->count;
}

void vector_add(vector* v,void* e)
{
	v->data = realloc(v->data,v->size + v->unit_size); /* reallocing the vector to increase the size */
	v->size += v->unit_size; 
	v->data[v->count] = e; /* storing the new data */
	v->count++; /* increasing the elements quantity */
}

void vector_set(vector* v,int index,void* e)
{
	if(index >= v->count)
		return;
	v->data[index] = e; /* reinitializing an element */
}

void* vector_get(vector* v,int index)
{
	if(index >= v->count)
		return NULL;
	return v->data[index]; /* getting an element with specific index */
}

void vector_delete(vector* v,int index)
{
	int i,j;
	void** new_data;
	if(index >= v->count)
		return;
	v->data[index] = NULL; 
	new_data = (void**)malloc(v->size); /* we want to copy the current datas to the new one */
	for(i = 0,j = 0; i < v->count;i++)
		if(v->data[i] != NULL) /* this is not the element */
		{
			new_data[j] = v->data[i]; /* copying the current datas to the new one */
			j++;
		}
	free(v->data);
	v->data = new_data;
	v->count--;
	v->size -= v->unit_size;
}

void vector_free(vector* v){
	free(v->data);
}
