#include <stdio.h>
#include "slab.h"
#include "buddy.h"
#include "cache.h"


int main() {

	int sz = 1000; //267 PUCA SA NEPARNIM BROJEM BLOKOVA WTFFF
	void* space = malloc(sz * BLOCK_SIZE);
	kmem_init(space, sz);


	void* ptr1 = kmalloc(31);
	void* ptr2 = kmalloc(50);

	kfree((char*)ptr1);
	kfree(ptr2);
	/*
	void* my_buff = kmalloc(32);
	void* my_buff1 = kmalloc(32);
	void* my_buff2 = kmalloc(32);
	void* my_buff3 = kmalloc(32);
	void* my_buff5 = kmalloc(32);
	void* my_buff6 = kmalloc(32);

	void* gg = kmalloc(116);
	void* gg1 = kmalloc(116);
	void* gg2 = kmalloc(116);
	void* gg3 = kmalloc(116);
	void* gg4 = kmalloc(116);
	void* gg5 = kmalloc(116);
	void* gg6 = kmalloc(116);*/
	
	kmem_cache_s* ch = kmem_cache_create("PCB", 69, NULL, NULL);
	kmem_cache_s* ch2 = kmem_cache_create("SEM", 70, NULL, NULL);
	kmem_cache_s* ch3 = kmem_cache_create("SEM", 30, NULL, NULL);
	void* objects[58];
	void* objects2[20];
	void* objects3[30];
	int i;

	for (i = 0; i < 58; i++)
		objects[i] = kmem_cache_alloc(ch);
	for (i = 0; i < 20; i++)
		objects2[i] = kmem_cache_alloc(ch2);
	for (i = 0; i < 30; i++)
		objects3[i] = kmem_cache_alloc(ch3);

	void* cache_overflow_ptr = kmem_cache_alloc(ch);

	printf("CACHE 1 INFO!!\n");
	print_cache(ch);
	printf("cache 2\n");
	print_cache(ch2);

	for (i = 0; i < 58; i++)
		kmem_cache_free(ch, objects[i]);
	for (i = 0; i < 20; i++)
		kmem_cache_free(ch2, objects2[i]);
	for (i = 0; i < 30; i++)
		kmem_cache_free(ch3, objects3[i]);

	print_cache(ch);

	printf("Freed %d blocks from cache!\n", kmem_cache_shrink(ch));
	printf("Freed %d blocks from cache!\n", kmem_cache_shrink(ch2));
	printf("Freed %d blocks from cache!\n", kmem_cache_shrink(ch3));
	print_cache(ch);

	kmem_cache_destroy(ch);
	kmem_cache_destroy(ch2);

	free(space);

	/*
	print_buddy();
	void* ptr1 = buddy_allocate(1);
	print_buddy();
	void* ptr2 = buddy_allocate(1);
	print_buddy();
	void* ptr3 = buddy_allocate(1);
	print_buddy();
	void* ptr4 = buddy_allocate(1);
	print_buddy();

	printf("\n\nposle dealloca\n");
	buddy_deallocate(ptr1, 1);
	print_buddy();

	void* ptr5 = buddy_allocate(1);
	print_buddy();
	*/
	return 0;
}