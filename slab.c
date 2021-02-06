#include <string.h>
#include <stdio.h>
#include <math.h>
#include <windows.h>
#include "slab.h"
#include "buddy.h"
#include "cache.h"

//pointer to the first cache
kmem_cache_s* cache_list_head = NULL;
//mutex used for sync
HANDLE mtx;

//int asked_for_eight = 0;
int info_called = 0;

void kmem_init(void* space, int block_num){

	//initializing the mutex
	mtx = CreateMutex(NULL, FALSE, NULL);
	if (mtx == NULL) {
		printf("unable to allocate mutex??");
		return;
	}

	//initalize the buddy allocator
	buddy_init(space, block_num);

	//initialize small memory buffers
	int i;
	for (i = 0; i < 13; i++) { //sizes from 2^5 to 2^17
		char buffer_name[30];
		sprintf_s(buffer_name, 30, "size-2^%d", i+5);
		int pw = i + 5;
		size_t size = (size_t)pow(2, pw);
		kmem_cache_create(buffer_name, size, NULL, NULL);
	}
}

kmem_cache_t* kmem_cache_create(const char* name, size_t size, void(*ctor)(void*), void(*dtor)(void*)){
	mutex_wait();
	void* ret = _kmem_cache_create(name, size, ctor, dtor);
	mutex_signal();
	return ret;
}

kmem_cache_t* _kmem_cache_create(const char* name, size_t size, void(*ctor)(void*), void (*dtor)(void*)) {
	//alocate a new cache manager
	kmem_cache_s* cache = (kmem_cache_s*)buddy_allocate(1);
	//error checking
	if (cache == NULL)
		return cache;
	
	//initializing the members
	cache->empty_slabs = NULL;
	cache->partial_slabs = NULL;
	cache->full_slabs = NULL;
	cache->count_empty = 0;
	cache->count_partial = 0;
	cache->count_full = 0;
	cache->obj_size = size;
	cache->num_of_allocated_objs = 0;
	cache->num_of_allocs_called = 0;
	cache->num_of_deallocs_called = 0;
	strcpy_s(cache->name, 30, name);
	cache->ctor = ctor;
	cache->dtor = dtor;
	cache->next = cache_list_head;

	//update the cache list
	cache_list_head = cache;

	return cache;
}

int kmem_cache_shrink(kmem_cache_t* cachep){
	mutex_wait();
	int ret = _kmem_cache_shrink(cachep);
	mutex_signal();
	return ret;
}

int _kmem_cache_shrink(kmem_cache_t* cachep) {
	int num_freed = 0;
	//check if the pointer is valid!
	if (!is_cache_valid(cachep)) {
		printf("No such cache exists!\n");
		return num_freed;
	}

	//find and deallocate all empty slabs
	slab_s* tmp = cachep->empty_slabs, * helper = NULL;
	while (tmp != NULL) {
		num_freed++;
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}

	//reset the empty slab list
	cachep->empty_slabs = NULL;
	cachep->count_empty = 0;

	return num_freed;
}

void* kmem_cache_alloc(kmem_cache_t* cachep){
	mutex_wait();
	void* ret = _kmem_cache_alloc(cachep);
	mutex_signal();
	return ret;
}

void* _kmem_cache_alloc(kmem_cache_t* cachep) {
	//check if the pointer is valid
	if (!is_cache_valid(cachep)) {
		printf("Cache err: No such cache exists");
		return NULL;
	}

	void* ret;
	//try to allocate an object in partialy filled slabs
	if (cachep->partial_slabs != NULL) {
		//print_slab(cachep->partial_slabs);
		ret = allocate_obj_from_slab(cachep->partial_slabs, cachep->ctor);
		//move the slab to full slab list if needed
		if (cachep->partial_slabs->state == 2) {
			slab_s* tmp = cachep->partial_slabs;
			cachep->partial_slabs = cachep->partial_slabs->next;
			tmp->next = cachep->full_slabs;
			cachep->full_slabs = tmp;
			cachep->count_partial--;
			cachep->count_full++;
		}
	}
	//try to allocate an object in empty slabs
	else if (cachep->empty_slabs != NULL) {
		ret = allocate_obj_from_slab(cachep->empty_slabs, cachep->ctor);
		//move the slab to partials and update empty list
		slab_s* tmp = cachep->empty_slabs;
		cachep->empty_slabs = cachep->empty_slabs->next;
		tmp->next = cachep->partial_slabs;
		cachep->partial_slabs = tmp;
		cachep->count_empty--;
		cachep->count_partial++;
	}

	else { //must allocate a new slab
		slab_s* tmp = allocate_a_slab(cachep->obj_size);
		//error checking!
		if (tmp == NULL) {
			printf("Err cache: Can't allocate a slab\n");
			return NULL;
		}
		
		ret = allocate_obj_from_slab(tmp, cachep->ctor);
		
		//update partials list
		tmp->next = cachep->partial_slabs;
		cachep->partial_slabs = tmp;
		cachep->count_partial++;
	}

	cachep->num_of_allocated_objs++;
	cachep->num_of_allocs_called++;
	return ret;
}

void kmem_cache_free(kmem_cache_t* cachep, void* objp) {
	mutex_wait();
	_kmem_cache_free(cachep, objp, 0);
	mutex_signal();
}

void* kmalloc(size_t size){
	mutex_wait();
	void* ret = _kmalloc(size);
	mutex_signal();
	return ret;
}

void* _kmalloc(size_t size) {
	kmem_cache_s* cp;
	//max value is 2^17
	if (size > (int)pow(2, 17)) {
		printf("Cache err: No buffer is that big!\n");
		return NULL;
	}
	int power = (int)ceil(log((double)size) / log(2.0));
	//min value is 2^5
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

	//more error checking, might be redundant!
	if (cp == NULL) {
		printf("Cache err: Could not find buffer!\n");
		return NULL;
	}

	//no space to allocate another 2^5 slab!
	void* ret = _kmem_cache_alloc(cp);
	if (ret == NULL)
		printf("Cache err: Can't allocate another slab for kmalloc!\n");

	return ret;
}

void kfree(const void* objp){
	mutex_wait();
	_kfree(objp);
	mutex_signal();
}

void _kfree(const void* objp) {

	kmem_cache_s* cp = cache_list_head;
	char firstBuffer[30];
	sprintf_s(firstBuffer, 30, "size-2^%d", 17); //17 is the first buffer in the cache list
	while (cp != NULL) {
		if (strcmp(cp->name, firstBuffer) == 0)
			break;
		cp = cp->next;
	}
	if (cp == NULL) {
		printf("Cache err: unable to find a buffer to free from!\n");
		return;
	}
	//try to dealloc from every buffer
	while (cp != NULL) {
		if (_kmem_cache_free(cp, objp, 1) == 0)
			return;
		cp = cp->next;
	}
	printf("Cache err: no such object allocated in buffers!\n");
}

void kmem_cache_destroy(kmem_cache_t* cachep){
	mutex_wait();
	_kmem_cache_destroy(cachep);
	mutex_signal();
}

void _kmem_cache_destroy(kmem_cache_t* cachep) {
	if (!is_cache_valid(cachep)) {
		printf("Cache err: No such cache exists to be destroyed\n");
		return;
	}
	//first deallocate all of the slabs
	slab_s* tmp, * helper;

	//dealloc empty slabs
	tmp = cachep->empty_slabs, helper = NULL;
	while (tmp != NULL) {
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}
	//deallocate partial slabs
	tmp = cachep->partial_slabs, helper = NULL;
	while (tmp != NULL) {
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}

	//deallocate full slabs
	tmp = cachep->full_slabs, helper = NULL;
	while (tmp != NULL) {
		helper = tmp->next;
		deallocate_a_slab(tmp);
		tmp = helper;
	}

	//deallocate the cache itself
	buddy_deallocate(cachep, 1);
}

void kmem_cache_info(kmem_cache_t* cachep){
	mutex_wait();
	_kmem_cache_info(cachep);
	printf("cache info called %d times\n", ++info_called);
	mutex_signal();
}

void _kmem_cache_info(kmem_cache_t* cachep) {
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

//helper function to see if the cachep is valid
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
	printf("cache num of allocs called: %d\n", c->num_of_allocs_called);
	printf("cache num of deallocs called: %d\n", c->num_of_deallocs_called);
	printf("cache num of objects per slab: %d\n", c->partial_slabs->capacity);
	printf("num of allocated objects: %d\n", c->num_of_allocated_objs);
	printf("\n");
}

int _kmem_cache_free(kmem_cache_t* cachep, void* objp, int is_internal){
	//is it a valid cache pointer
	if (!is_cache_valid(cachep)) {
		printf("Cache err: Unable to dealloc object, no such cache exists\n");
		return -1;
	}

	//is the object in the partials
	slab_s* tmp = cachep->partial_slabs, * prev = NULL;
	while (tmp != NULL) {
		//try to dealloc from a slab
		//if the obj is deallocated the function returns 0
		if (dealloc_obj_from_slab(tmp, objp) == 0) {
			cachep->num_of_allocated_objs--;
			//move to empty list if needed
			if (tmp->state == 0) {
				//if this is the first partial - meaning the only partial, reset the partial pointer
				if (tmp == cachep->partial_slabs)
					cachep->partial_slabs = cachep->partial_slabs->next;
				//if not just move the pointer
				else
					prev->next = tmp->next;
				tmp->next = cachep->empty_slabs;
				cachep->empty_slabs = tmp;
				cachep->count_partial--;
				cachep->count_empty++;
			}
			cachep->num_of_deallocs_called++;
			return 0;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	//try to dealloc from full slabs
	tmp = cachep->full_slabs;
	prev = NULL;
	while (tmp != NULL) {
		if (dealloc_obj_from_slab(tmp, objp) == 0) {
			cachep->num_of_allocated_objs--;
			//move the slab to partials

			//check if this is the list head
			if (tmp == cachep->full_slabs)
				cachep->full_slabs = cachep->full_slabs->next;
			//if it's not just move the pointer
			else
				prev->next = tmp->next;

			tmp->next = cachep->partial_slabs;
			cachep->partial_slabs = tmp;
			cachep->count_partial++;
			cachep->count_full--;

			cachep->num_of_deallocs_called++;
			return 0;
		}
		prev = tmp;
		tmp = tmp->next;
	}

	//if the user called this function, print an error
	if(is_internal == 0)
		printf("Cache err: No such object exists!\n");
	return -1;
}

//mutex sync functions
void mutex_wait() { WaitForSingleObject(mtx, INFINITE);}
void mutex_signal() { ReleaseMutex(mtx); }

