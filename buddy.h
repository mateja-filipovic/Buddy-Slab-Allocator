#ifndef _BUDDY_H_
#define _BUDDY_H_

#include "slab.h"

#define POOL_MAX_SIZE 32

typedef struct buddy_s {
	void* base_addr;
	void* starting_addr;
	void* ending_addr;
	unsigned long capacity;
	unsigned int max_order;
	void* pool[32];
}buddy_s;

void buddy_init(void* space, int numOfBlocks);
void print_buddy();
void* buddy_allocate(int sizeInBlocks);
void buddy_deallocate(void* blkAddr, int sizeInBlocks);
int get_blk_size(int i);
void* get_buddy_of(void* b, int i);

#endif