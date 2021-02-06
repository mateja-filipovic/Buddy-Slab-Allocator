#ifndef _BUDDY_H_
#define _BUDDY_H_

#include "slab.h"

#define POOL_MAX_SIZE 32

typedef struct buddy_s {
	void* base_addr;
	void* starting_addr;
	void* ending_addr;
	unsigned long blocks_total;
	unsigned int max_order;
	void* pool[32];
}buddy_s;

void buddy_init(void* space, int numOfBlocks); //initalize
void print_buddy(); //print info
void* buddy_allocate(int sizeInBlocks); //allocate space given block size needed
void buddy_deallocate(void* blkAddr, int sizeInBlocks); //deallocate space given startign address adn block size needed

//helper functions
int _get_blk_size(int i);
void* _get_buddy_of(void* b, int i);

#endif