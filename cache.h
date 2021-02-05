#ifndef _CACHE_H_
#define _CACHE_H_

#include "buddy.h"

typedef struct slab_s {
	void* base_addr;
	void* starting_addr;
	void* ending_addr;
	void* first_free;
	int state; // 0 - empty, 1 - partial, 2 - full
	int num_of_allocs;
	int capacity;
	int obj_size;
	struct slab_s* next;
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
	char name[30];
	void (*ctor)(void*);
	void (*dtor)(void*);
	struct kmem_cache_s* next;
}kmem_cache_s;



void print_slab(slab_s* sl);
void* allocate_obj_from_slab(slab_s* sl);
void deallocate_a_slab(slab_s* sl);
int dealloc_obj_from_slab(slab_s* sl, void* obj);
int is_obj_valid(slab_s* sl, void* obj);

slab_s* allocate_a_slab(int obj_size);
#endif