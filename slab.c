#include <string.h>
#include <stdio.h>
#include "slab.h"
#include "buddy.h"
#include "cache.h"

kmem_cache_s* cache_list_head = NULL;

void kmem_init(void* space, int block_num){
	buddy_init(space, block_num);
}

kmem_cache_t* kmem_cache_create(const char* name, size_t size, void(*ctor)(void*), void(*dtor)(void*)){
	kmem_cache_s* cache = (kmem_cache_s*)buddy_allocate(1);
	if (cache == NULL)
		return cache;
	cache->empty_slabs = NULL;
	cache->partial_slabs = NULL;
	cache->full_slabs = NULL;
	cache->count_empty = 0;
	cache->count_partial = 0;
	cache->count_full = 0;
	cache->obj_size = size;
	cache->num_of_allocated_objs = 0;
	strcpy_s(cache->name, 30, name);
	cache->ctor = ctor;
	cache->dtor = dtor;
	cache->next = cache_list_head;
	cache_list_head = cache;
	return cache;
}

int kmem_cache_shrink(kmem_cache_t* cachep){
	int num_freed = 0;
	if (!is_cache_valid(cachep)) {
		printf("No such cache exists!\n");
		return num_freed;
	}
	slab_s* tmp = cachep->empty_slabs, *helper = NULL;
	while (tmp != NULL) {
		num_freed++;
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}
	cachep->empty_slabs = NULL;
	cachep->count_empty = 0;
	return num_freed;
}

void* kmem_cache_alloc(kmem_cache_t* cachep){
	if (!is_cache_valid(cachep)) {
		printf("Cache err: No such cache exists");
		return NULL;
	}
	void* ret;
	if(cachep->partial_slabs != NULL){
		//print_slab(cachep->partial_slabs);
		ret = allocate_obj_from_slab(cachep->partial_slabs);
		if (cachep->partial_slabs->state == 2) {
			slab_s* tmp = cachep->partial_slabs;
			cachep->partial_slabs = cachep->partial_slabs->next;
			tmp->next = cachep->full_slabs;
			cachep->full_slabs = tmp;
			cachep->count_partial--;
			cachep->count_full++;
		}
	}
	else if (cachep->empty_slabs != NULL) {
		ret = allocate_obj_from_slab(cachep->empty_slabs);
		slab_s* tmp = cachep->empty_slabs;
		cachep->empty_slabs = cachep->empty_slabs->next;
		tmp->next = cachep->partial_slabs;
		cachep->partial_slabs = tmp;
		cachep->count_empty--;
		cachep->count_partial++;
	}
	else {
		slab_s* tmp = allocate_a_slab(cachep->obj_size);
		//print_slab(tmp);
		if (tmp == NULL) {
			printf("Err cache: Can't allocate a slab\n");
			return NULL;
		}
		ret = allocate_obj_from_slab(tmp);
		tmp->next = cachep->partial_slabs;
		cachep->partial_slabs = tmp;
		cachep->count_partial++;
	}
	cachep->num_of_allocated_objs++;
	return ret;
}

void kmem_cache_free(kmem_cache_t* cachep, void* objp) {
	if (!is_cache_valid(cachep)) {
		printf("Cache err: Unable to dealloc object, no such cache exists\n");
		return;
	}


	//da li se nalazi u parcijalno popunjenim?
	slab_s* tmp = cachep->partial_slabs, *prev = NULL;
	while (tmp != NULL) {
		if (dealloc_obj_from_slab(tmp, objp) == 0) {
			//printf("object deallocated successfuly\n");
			cachep->num_of_allocated_objs--;
			//ako se ispraznio prebaci ga u prazne
			if (tmp->state == 0) {
				//ako je prvi u listi prevezi head i prebaci ga u prazne
				if (tmp == cachep->partial_slabs)
					cachep->partial_slabs = cachep->partial_slabs->next;
				//ako ne samo prevezi prev pokazivac
				else
					prev->next = tmp->next;
				tmp->next = cachep->empty_slabs;
				cachep->empty_slabs = tmp;
				cachep->count_partial--;
				cachep->count_empty++;
			}
			return;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	tmp = cachep->full_slabs;
	prev = NULL;
	while (tmp != NULL) {
		if (dealloc_obj_from_slab(tmp, objp) == 0) {
			//printf("object deallocated successfuly\n");
			cachep->num_of_allocated_objs--;
			//prebaci ga u partials
			//ako je prvi u listi prevezi head i prebaci ga u prazne
			if (tmp == cachep->full_slabs)
				cachep->full_slabs = cachep->full_slabs->next;
			//ako ne samo prevezi prev pokazivac
			else
				prev->next = tmp->next;
			tmp->next = cachep->partial_slabs;
			cachep->partial_slabs = tmp;
			cachep->count_partial++;
			cachep->count_full--;
			return;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	printf("Cache err: No such object exists!\n");
}

void kmem_cache_destroy(kmem_cache_t* cachep){
	if (!is_cache_valid(cachep)) {
		printf("Cache err: No such cache exists to be destroyed\n");
		return;
	}
	//prvo obrisati sve alocirane slabove!
	slab_s* tmp, * helper;
	tmp = cachep->empty_slabs, helper = NULL;
	while (tmp != NULL) {
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}
	tmp = cachep->partial_slabs, helper = NULL;
	while (tmp != NULL) {
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}
	tmp = cachep->full_slabs, helper = NULL;
	while (tmp != NULL) {
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}
	buddy_deallocate(cachep, 1);
}

int is_cache_valid(void* cachep){
	kmem_cache_s* tmp = cache_list_head;
	while (tmp != NULL) {
		if (tmp == cachep)
			return 1;
		tmp = tmp->next;
	}
	return 0;
}

void print_cache(kmem_cache_s* c){
	printf("cache starting addr: %p\n", c);
	printf("cache ending addr: %p\n", (char*)c + BLOCK_SIZE);
	printf("cache num of empty slabs: %d\n", c->count_empty);
	printf("cache num of partial slabs: %d\n", c->count_partial);
	printf("cache num of full slabs: %d\n", c->count_full);
	printf("\n");
}