#include <stdio.h>
#include "cache.h"

#include "slab.h"


slab_s* allocate_a_slab(int obj_size){
	slab_s* sl = buddy_allocate(1);
	sl->base_addr = sl;
	sl->starting_addr = (char*)sl->base_addr + sizeof(slab_s);
	sl->ending_addr = (char*)sl->base_addr + BLOCK_SIZE;
	sl->first_free = sl->starting_addr;
	sl->state = 0; // 0 - empty, 1 - partial, 2 - full
	sl->num_of_allocs = 0;
	sl->capacity = (BLOCK_SIZE - sizeof(slab_s)) / obj_size;
	sl->obj_size = obj_size;
	sl->next = NULL;

	int i;
	void** tmp = (void**)sl->first_free;
	for (i = 0; i < sl->capacity; i++) {
		*tmp = (char*)(tmp)+obj_size;
		if (i == sl->capacity - 1) {
			*tmp = 0;
			break; //break da se ne bi bunio za nullptr exception
		}
		tmp = *tmp;
	}
	return sl;
}

void print_slab(slab_s* sl) {
	printf("slab base addr: %p\n", sl->base_addr);
	printf("slab starting addr: %p\n", sl->starting_addr);
	printf("slab ending addr: %p\n", sl->ending_addr);
	printf("slab capacity: %d, allocated so far: %d\n", sl->capacity, sl->num_of_allocs);
	printf("\n");
}

void* allocate_obj_from_slab(slab_s* sl){
	if (sl->capacity == sl->num_of_allocs)
		return NULL;
	void* ret = sl->first_free;
	sl->first_free = *(void**)sl->first_free;
	sl->num_of_allocs++;
	if (sl->state == 0)
		sl->state = 1;
	else if (sl->num_of_allocs == sl->capacity)
		sl->state = 2;
	return ret;
}

void deallocate_a_slab(slab_s* sl){
	buddy_deallocate(sl, 1);
}

int dealloc_obj_from_slab(slab_s* sl, void* obj){
	if (sl->num_of_allocs == 0)
		return -1; //no objs allocated
	if (!is_obj_valid(sl, obj))
		return -2; //the obj is not from this slab
	*(void**)obj = sl->first_free;
	sl->first_free = obj;
	sl->num_of_allocs--;
	if (sl->state == 2)
		sl->state = 1;
	else if (sl->num_of_allocs == 0)
		sl->state = 0;
	return 0;
	//should obj pointer be reset?
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

