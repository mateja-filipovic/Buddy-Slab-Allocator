#include <stdio.h>
#include <stdlib.h>
#include "cache.h"
#include <string.h>

#include "slab.h"

#define MAX_VECT_SIZE 512 //512 * 8 = 4096 maximum

slab_s* allocate_a_slab(int obj_size, int* objs_per_slab_ret, void(*ctor)(void*), int* wasted_ret, int offs) {
	//get the number of blocks needed
	int block_num = (10 * obj_size) / BLOCK_SIZE;
	if (10 * obj_size % BLOCK_SIZE != 0)
		block_num++;

	slab_s* sl = buddy_allocate(1);
	//error checking
	if (sl == NULL) {
		printf("Slab error: Buddy has no space to allocate another slab manager!\n");
		return NULL;
	}

	sl->base_addr = buddy_allocate(block_num);
	if (sl->base_addr == NULL) {
		printf("Slab error: Can't allocate another slab!\n");
		return NULL;
	}

	sl->starting_addr = (char*)sl->base_addr + offs;      //always cast to char* cos of pointer arithmetic!!
	sl->ending_addr = (char*)sl->base_addr + BLOCK_SIZE * block_num;

	if(*objs_per_slab_ret == -1)
		*objs_per_slab_ret = (block_num * BLOCK_SIZE - sizeof(slab_s)) / obj_size;
	if (*wasted_ret == -1) {
		*wasted_ret = (block_num * BLOCK_SIZE) % obj_size;
	}

	sl->num_of_blocks = block_num;
	sl->capacity = (block_num * BLOCK_SIZE) / obj_size;
	sl->obj_size = obj_size;

	sl->num_of_allocs = 0;
	sl->state = 0;

	sl->next = NULL; //used by the cache manager!
	sl->bit_vector = (unsigned char*)sl + sizeof(slab_s);
	
	memset(sl->bit_vector, 0, MAX_VECT_SIZE); //svi su slobodni

	//initialize the objects using the ctor provided!
	sl->ctor = ctor;
	
	int i;
	if (ctor != NULL) {
		for (i = 0; i < sl->capacity; i++) {
			void* objp = get_obj_address(sl, i);
			if (objp < sl->starting_addr || objp >= sl->ending_addr)
				printf("Slab error: Cannot construct object!");
			else
				(*ctor)(objp);
		}
	}

	return sl;
}

void print_slab(slab_s* sl) {
	printf("Slab base addr: %p\n", sl->base_addr);
	printf("Slab starting addr: %p\n", sl->starting_addr);
	printf("Slab ending addr: %p\n", sl->ending_addr);
	printf("Slab capacity: %d, allocated so far: %d, obj size: %d\n", sl->capacity, sl->num_of_allocs, sl->obj_size);
}

void* allocate_obj_from_slab(slab_s* sl) {
	if (sl->capacity == sl->num_of_allocs)
		return NULL;

	int i = 0;
	while (get_bit(sl, i) != 0)
		i++;

	void* ret = get_obj_address(sl, i);
	set_bit(sl, i, 1);

	sl->num_of_allocs++;
	//change the state if necessary
	if (sl->state == 0)
		sl->state = 1;
	else if (sl->num_of_allocs == sl->capacity) //|| sl->first_free == 0) //second condition added, more error checking!
		sl->state = 2;

	//just in case a dealloced object gets allocated again
	//if (sl->ctor != NULL)
		//(*sl->ctor)(ret);

	return ret;
}

void deallocate_a_slab(slab_s* sl){
	buddy_deallocate(sl->base_addr, sl->num_of_blocks);
	buddy_deallocate((void*)sl, 1);

}

int dealloc_obj_from_slab(slab_s* sl, void* obj, void(*dtor)(void*)){
	
	if (sl->num_of_allocs == 0)
		return -1; //no objs allocated
	int obj_index = get_obj_index(sl, obj);
	if (obj_index == -1)
		return -2; //the obj is not from this slab



	//update the free slots list
	set_bit(sl, obj_index, 0);

	sl->num_of_allocs--;
	//change the state if necessary
	if (sl->state == 2)
		sl->state = 1;
	else if (sl->num_of_allocs == 0)
		sl->state = 0;

	/*
	* NOTE: PROJECT REQS UNCLEAR, NOT SURE IF AN OBJECT SHOULD BE INITIALIZED
	* AFTER BEING DEALLOCATED, OR JUST DESTRUCTED
	* UNCOMMENT IF NECESSARY
	* if (sl->ctor != NULL)
		* (*sl->ctor)(obj);
	*/

	return 0;
}

//get the bit vector index of an object slot
int get_obj_index(slab_s* sl, void* obj) {
	int index = -1;
	if (obj >= sl->ending_addr || obj < sl->starting_addr)
		return index;
	int calc = (unsigned int)(obj)-(unsigned int)(sl->starting_addr);
	if (calc % sl->obj_size != 0)
		return index;
	return calc / sl->obj_size;
}

//get object address using a bit vector index
void* get_obj_address(slab_s* sl, int index){
	void* ret = (char*)sl->starting_addr + index * sl->obj_size;
	return ret;
}

void print_bit_vect(slab_s* sl){
	char* bv = sl->bit_vector;
	int i;
	for (i = 0; i < sl->capacity; i++) {
		printf("%d", get_bit(sl, i));
		if ((i+1) % 8 == 0 && i != 0)
			printf(", ");
	}
	printf("\n");
}

void set_bit(slab_s* sl, int index, int bit) {
	//find the word
	int word = index >> 3;
	int n = sizeof(index) * 8 - 3;
	int offset = ((unsigned)index << n) >> n;
	if (bit) {
		sl->bit_vector[word] |= 1 << (7 - offset);
	}
	else {
		sl->bit_vector[word] &= ~(1 << (7 - offset));
	}
}

int get_bit(slab_s* sl, int index) {
	//find the word
	int word = index >> 3;
	int n = sizeof(index) * 8 - 3;
	int offset = ((unsigned)index << n) >> n;
	//check what value the bit has
	if (sl->bit_vector[word] & (1 << (7 - offset))) {
		return 1;
	}
	else {
		return 0;
	}
}

