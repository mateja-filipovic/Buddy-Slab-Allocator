#include <string.h>
#include <stdio.h>
#include <math.h>
#include "slab.h"
#include "buddy.h"
#include "cache.h"

kmem_cache_s* cache_list_head = NULL;

void kmem_init(void* space, int block_num){
	buddy_init(space, block_num);

	int i;
	for (i = 0; i < 13; i++) { //broj malih memorijskih bafera 2^5 do 2^17
		
		char buffer_name[30];
		sprintf_s(buffer_name, 30, "size-2^%d", i+5);
		printf("initialized: %s\n", buffer_name);
		int pw = i + 5;
		size_t size = pow(2, pw);
		kmem_cache_create(buffer_name, size, NULL, NULL);
	}
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
	_kmem_cache_free(cachep, objp, 0);
}

void* kmalloc(size_t size){
	kmem_cache_s* cp;
	if (size > (int)pow(2, 17)) {
		printf("Cache err: No buffer is that big!\n");
		return NULL;
	}
	int power = (int)ceil(log((double)size) / log(2.0));
	if (power < 5)
		power = 5;
	//find the buffer
	cp = cache_list_head;
	char buffer_name[30];
	sprintf_s(buffer_name, 30, "size-2^%d", power);
	while (cp != NULL) {
		if (strcmp(buffer_name, cp->name) == 0)
			break;
		cp = cp->next;
	}
	if(cp == NULL) {
		printf("Cache err: Could not find buffer!\n");
		return NULL;
	}
	printf("Nasao cache! %p\n", cp);
	void* ret = kmem_cache_alloc(cp);
	if (ret == NULL)
		printf("Cache err: No space available in buffer!\n");
	return ret;
}

void kfree(const void* objp){
	kmem_cache_s* cp = cache_list_head;
	char firstBuffer[30];
	sprintf_s(firstBuffer, 30, "size-2^%d", 17); //17 je max velicina bafera
	while (cp != NULL) {
		if (strcmp(cp->name, firstBuffer) == 0)
			break;
		cp = cp->next;
	}
	if (cp == NULL) {
		printf("Cache err: unable to find a buffer to free from!\n");
		return;
	}
	while (cp != NULL) {
		if (_kmem_cache_free(cp, objp, 1) == 0)
			return;
		cp = cp->next;
	}
	printf("Cache err: no such object allocated in buffers!\n");
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

void kmem_cache_info(kmem_cache_t* cachep){
	kmem_cache_s* cp = cache_list_head;
	while (cp != NULL) {
		if (cp == cachep)
			break;
		cp = cp->next;
	}
	if (cp == NULL) {
		printf("Cache err: Cannot print, such cache does not exist!\n");
		return;
	}
	print_cache(cachep);
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

int _kmem_cache_free(kmem_cache_t* cachep, void* objp, int is_internal){

	if (!is_cache_valid(cachep)) {
		printf("Cache err: Unable to dealloc object, no such cache exists\n");
		return -1;
	}

	//da li se nalazi u parcijalno popunjenim?
	slab_s* tmp = cachep->partial_slabs, * prev = NULL;
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
			return 0;
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
			return 0;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	if(is_internal == 0)
		printf("Cache err: No such object exists!\n");
	return -1;
}
