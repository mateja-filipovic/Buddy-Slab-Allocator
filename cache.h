#ifndef _CACHE_H_
#define _CACHE_H_

#include "buddy.h"

#define SMALL_MEM_BUFFER_NUM

typedef struct slab_s {
	void* base_addr;
	void* starting_addr;
	void* ending_addr;
	void* first_free;
	//unsigned int first_free;
	int num_of_blocks;
	int capacity;
	int state; // 0 - empty, 1 - partial, 2 - full
	int num_of_allocs; //objects allocated so far
	int obj_size;
	struct slab_s* next; //used by the cache manager
}slab_s;

typedef struct kmem_cache_s{
	slab_s* empty_slabs;
	slab_s* partial_slabs;
	slab_s* full_slabs;
	int count_empty;
	int count_partial;
	int count_full;
	int obj_size;
	int num_of_allocated_objs;
	int objs_per_slab;
	int num_of_blocks;
	int num_of_errors;
	char name[30];
	void (*ctor)(void*);
	void (*dtor)(void*);
	struct kmem_cache_s* next;
}kmem_cache_s;

slab_s* allocate_a_slab(int obj_size, int* ret_param); //allocate a new slab
void deallocate_a_slab(slab_s* sl); //free a slab

void print_slab(slab_s* sl); //print slab info

void* allocate_obj_from_slab(slab_s* sl, void(*ctor)(void*)); //allocate an object and initialize it
int dealloc_obj_from_slab(slab_s* sl, void* obj, void(*dtor)(void*)); //return 0 if dealloc is successful

//helper function
int is_obj_valid(slab_s* sl, void* obj); //check if the pointer to dealloc is valid

#endif