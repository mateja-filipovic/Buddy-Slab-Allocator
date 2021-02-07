#include <stdio.h>
#include "cache.h"

#include "slab.h"

#define MIN_OBJ_SIZE 4


slab_s* allocate_a_slab(int obj_size, int* ret_param) {
	//get the number of blocks needed
	int block_num = (sizeof(slab_s) + 10 * obj_size) / BLOCK_SIZE;
	if (sizeof(slab_s) + 10 * obj_size % BLOCK_SIZE != 0)
		block_num++;

	slab_s* sl = buddy_allocate(block_num);
	//error checking
	if (sl == NULL) {
		printf("Slab err: Buddy has no space to allocate another slab!\n");
		return NULL;
	}

	sl->base_addr = sl;
	sl->starting_addr = (char*)sl->base_addr + sizeof(slab_s);      //always cast to char* cos of pointer arithmetic!!
	sl->ending_addr = (char*)sl->base_addr + BLOCK_SIZE * block_num;
	sl->first_free = sl->starting_addr;

	*ret_param = (block_num * BLOCK_SIZE - sizeof(slab_s)) / obj_size;

	if (obj_size < MIN_OBJ_SIZE)
		obj_size = MIN_OBJ_SIZE;
	sl->num_of_blocks = block_num;
	sl->capacity = (block_num * BLOCK_SIZE - sizeof(slab_s)) / obj_size;
	sl->obj_size = obj_size;

	sl->num_of_allocs = 0;
	sl->state = 0;

	sl->next = NULL; //used by the cache manager!
	
	//initialize the free blocks list
	int i;
	void** tmp = (void**)sl->first_free; //tmp stores the address of the free block
	for (i = 0; i < sl->capacity; i++) {
		*tmp = (char*)(tmp) + obj_size; //store the address of the next free block in the current free block
		if (i == sl->capacity - 1) {
			*tmp = 0; //the last free block points to 0
			break; //to handle a nullptr warning
		}
		tmp = *tmp;
	}
	return sl;
}

void print_slab(slab_s* sl) {
	printf("Slab base addr: %p\n", sl->base_addr);
	printf("Slab starting addr: %p\n", sl->starting_addr);
	printf("Slab ending addr: %p\n", sl->ending_addr);
	printf("Slab capacity: %d, allocated so far: %d, obj size: %d\n", sl->capacity, sl->num_of_allocs, sl->obj_size);
}

void* allocate_obj_from_slab(slab_s* sl, void(*ctor)(void*)){
	if (sl->capacity == sl->num_of_allocs)
		return NULL;


	void* ret = sl->first_free;
	//change
	if (sl->first_free != 0)
		sl->first_free = *(void**)sl->first_free;
	///chagne end, maybe needs a revert!

	//sl->first_free = *(void**)sl->first_free;
	//sl->num_of_allocs++;

	sl->num_of_allocs++;
	//change the state if necessary
	if (sl->state == 0)
		sl->state = 1;
	else if (sl->num_of_allocs == sl->capacity || sl->first_free == 0) //second condition added, more error checking!
		sl->state = 2;

	//initalize the object if the constructor is provided
	if (ctor != NULL)
		(*ctor)(ret);

	return ret;
}

void deallocate_a_slab(slab_s* sl){
	buddy_deallocate(sl->base_addr, sl->num_of_blocks);
	//REVERT IF NEEDED
	//unsigned int sz = (unsigned int)sl->ending_addr - (unsigned int)sl->base_addr;
	//buddy_deallocate(sl->base_addr, sz);
	//REVERT IF NEEDED

}

int dealloc_obj_from_slab(slab_s* sl, void* obj, void(*dtor)(void*)){
	
	if (sl->num_of_allocs == 0)
		return -1; //no objs allocated
	if (!is_obj_valid(sl, obj))
		return -2; //the obj is not from this slab

	//update the free slots list
	*(void**)obj = sl->first_free;
	sl->first_free = obj;

	sl->num_of_allocs--;
	//change the state if necessary
	if (sl->state == 2)
		sl->state = 1;
	else if (sl->num_of_allocs == 0)
		sl->state = 0;

	if (dtor != NULL)
		(*dtor)(obj);

	return 0;
}

int is_obj_valid(slab_s* sl, void* obj) {
	void* tmp = sl->starting_addr;
	while (tmp < sl->ending_addr) {
		if (obj == tmp)
			return 1; //object is from this slab
		//check if an object is already free?
		tmp = (char*)tmp + sl->obj_size;
	}
	return 0;
}

