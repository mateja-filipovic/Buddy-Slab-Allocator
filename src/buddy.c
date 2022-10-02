#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../inc/buddy.h"

buddy_s* BUDDY;

void buddy_init(void* space, int numOfBlocks) {
	
	//set everything to 0
	memset(space, 0, numOfBlocks * BLOCK_SIZE);
	BUDDY = (buddy_s*)space;

	//initialize struct memebers
	BUDDY->base_addr = space;
	BUDDY->starting_addr = (char*)space + BLOCK_SIZE;
	BUDDY->ending_addr = (char*)space + numOfBlocks * BLOCK_SIZE;
	BUDDY->blocks_total = numOfBlocks - 1;
	BUDDY->max_order = (int)ceil(log(BUDDY->blocks_total) / log(2));

	//initialize the pool to 0
	int i;
	for (i = 0; i < POOL_MAX_SIZE; i++)
		BUDDY->pool[i] = 0;

	//split the remaining space and save addresses in the pool
	int temp_cap = BUDDY->blocks_total;
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

void* buddy_allocate(int sizeInBlocks){
	int i, order;
	void* block, * bud;
	i = 0;

	//find the minimum size in blocks
	while (_get_blk_size(i) < sizeInBlocks)
		i++;
	order = i;

	//find the first big enough chunk available
	for (;; i++) {
		if (i > BUDDY->max_order)
			return NULL;
		if (BUDDY->pool[i])
			break;
	}

	block = BUDDY->pool[i];
	BUDDY->pool[i] = *(void**)BUDDY->pool[i];
	while (i-- > order) {
		bud = _get_buddy_of(block, i);
		BUDDY->pool[i] = bud;
	}
	return block;
}

void buddy_deallocate(void* blkAddr, int sizeInBlocks){
	int i;

	//find the order of the block
	for (i = 0; _get_blk_size(i) < sizeInBlocks; i++);
	void* bud = _get_buddy_of(blkAddr, i);

	//add it to the pool of free blocks
	void** p = &BUDDY->pool[i];

	//find the buddy in the same pool index
	while ((*p != NULL) && (*p != bud))
		p = (void**)*p;

	//if buddy is not found/free
	if (bud > BUDDY->ending_addr || *p != bud) {
		*(void**)blkAddr = BUDDY->pool[i];
		BUDDY->pool[i] = blkAddr;
	}
	else { //if the buddy is free
		if (bud != 0) { //this check added for nullptr warning, REVERT IF NEEDED, but should not be a problem
			*p = *(void**)bud;
			if (blkAddr > bud) //if the buddy is free, dealloc the bigger block starting from buddy address
				buddy_deallocate(bud, _get_blk_size(i + 1));
			else //if the buddy is free, dealloc the bigger block starting from the starting block addr
				buddy_deallocate(blkAddr, _get_blk_size(i + 1));
		}
	}
}


//helper functions
int _get_blk_size(int i) { return 1 << i; }

void* _get_buddy_of(void* b, int i) { 
	//for the algorithm to work, heap start must be 0
	//so offset the starting address by buddy->starting_addr
	int tmp = (int)b - (int)BUDDY->starting_addr;
	tmp ^= (1 << (i + 12)); //must add +12 cos block size is 4096 = 2^12
	//after the calc is finished, add the offset back!
	void* ret = (void*)((char*)BUDDY->starting_addr + tmp);
	//error checking
	if (ret > BUDDY->ending_addr)
		return 0;
	return ret;
}
