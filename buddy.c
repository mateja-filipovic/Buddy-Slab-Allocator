#include <math.h>
#include <stdio.h>
#include <string.h>
#include "buddy.h"

buddy_s* BUDDY;
void buddy_init(void* space, int numOfBlocks) {
	
	memset(space, 0, numOfBlocks * BLOCK_SIZE);
	BUDDY = (buddy_s*)space;
	BUDDY->base_addr = space;
	BUDDY->starting_addr = (char*)space + BLOCK_SIZE;
	BUDDY->ending_addr = (char*)space + numOfBlocks * BLOCK_SIZE;
	//if (numOfBlocks % 2 == 1) numOfBlocks--; //vratiti ako baguje!!
	BUDDY->capacity = numOfBlocks - 1;
	BUDDY->max_order = (int)ceil(log(BUDDY->capacity) / log(2));
	int i;
	for (i = 0; i < POOL_MAX_SIZE; i++)
		BUDDY->pool[i] = 0;
	int temp_cap = BUDDY->capacity;
	void* temp_addr = BUDDY->starting_addr;
	i = BUDDY->max_order;
	while (temp_cap > 0) {
		int mvmnt = (1 << i);
		if (temp_cap - mvmnt < 0) {
			i--;
			continue;
		}
		BUDDY->pool[i] = temp_addr;
		temp_addr = (char*)temp_addr + (mvmnt * BLOCK_SIZE);
		i--;
		temp_cap -= mvmnt;
	}
}

void print_buddy() {
	printf("Buddy base address: %p\n", BUDDY->base_addr);
	printf("Buddy start address: %p\n", BUDDY->starting_addr);
	printf("Buddy end address: %p\n", BUDDY->ending_addr);
	printf("Buddy block capacity: %d\n", BUDDY->capacity);
	unsigned int i;
	for (i = 0; i < BUDDY->max_order + 1; i++)
		printf("%d -> %p\n", i, BUDDY->pool[i]);
}

void* buddy_allocate(int sizeInBlocks){
	int i, order;
	void* block, * bud;
	i = 0;
	while (get_blk_size(i) < sizeInBlocks)
		i++;
	order = i;
	for (;; i++) {
		if (i > BUDDY->max_order)
			return NULL;
		if (BUDDY->pool[i])
			break;
	}
	block = BUDDY->pool[i];
	BUDDY->pool[i] = *(void**)BUDDY->pool[i];
	while (i-- > order) {
		bud = get_buddy_of(block, i);
		BUDDY->pool[i] = bud;
	}
	return block;
	/*
	int i;
	for (i = 0; get_blk_size(i) < sizeInBlocks; i++); //dodato *4096
	if (i >= BUDDY->max_order+1) {
		printf("Buddy err: No space available\n");
		return NULL;
	}
	else if (BUDDY->pool[i] != NULL) {
		void* block = BUDDY->pool[i];
		BUDDY->pool[i] = *(void**)BUDDY->pool[i];
		return block;
	}
	else {
		void* block = buddy_allocate(get_blk_size(i + 1)); //dodato
		if (block != NULL) {
			void* bud = get_buddy_of(block, i);
			if (bud != NULL) {
				*(void**)bud = BUDDY->pool[i];
				BUDDY->pool[i] = bud;
			}
		}
		return block;
	}*/
}

void buddy_deallocate(void* blkAddr, int sizeInBlocks){
	int i;
	for (i = 0; get_blk_size(i) < sizeInBlocks; i++);
	void* bud = get_buddy_of(blkAddr, i);
	void** p = &BUDDY->pool[i];
	while ((*p != NULL) && (*p != bud))
		p = (void**)*p;
	if (bud > BUDDY->ending_addr || *p != bud) {
		*(void**)blkAddr = BUDDY->pool[i];
		BUDDY->pool[i] = blkAddr;
	}
	else {
		*p = *(void**)bud;
		if (blkAddr > bud)
			buddy_deallocate(bud, get_blk_size(i + 1)); //dodato
		else
			buddy_deallocate(blkAddr, get_blk_size(i + 1)); //dodato
	}
}

int get_blk_size(int i) { return 1 << i; }

void* get_buddy_of(void* b, int i) { 
	//return (void*)(((int)b) ^  (1 << (i)) ); 
	int tmp = (int)b - (int)BUDDY->starting_addr; //namesti offset heap-a na nula
	tmp ^= (1 << (i + 12));
	void* ret = (void*)((char*)BUDDY->starting_addr + tmp);
	/*if (ret < BUDDY->starting_addr)
		return NULL;*/
	if (ret > BUDDY->ending_addr)
		return 0;
	//printf("%p asked for buddy, found: %p, i = %d\n", b, ret, i);
	return ret;
} //dodato +13 jer 4096 = 2^13
