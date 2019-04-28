/*
 * Malloc implementation consists of two explicit lists. One to keep track of the allocated memory
 * and one to keep track of the freed memory. Essentially, first the program looks in the freed list to see 
 * a chunk can be found that is relatively similar in size as not to waste memory allocating a big chunk for
 * only a small request. However, if nothing is found in the free list, sbrk is called and the memory is 
 * given directly from the system. The average of the test cases given is 70.9% in relation to the system malloc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

#define MIN_GUARD 1024

//Node stucture for doubly linked list
typedef struct header {
	size_t size;
	struct header *next;
	struct header *prev;
	size_t status;  
} header;


//head reference to the allocated list
header *global_allocated_list_head= NULL;

//head reference to the free list
header *global_free_list = NULL;

//function to quickly and easily add/remove asserts
void my_assert(bool condition) {
	//assert(condition);
}

//function from naive changed to represent my header struct
//Easily sets size and status(allocated or freed) of a given chunk
void set_chunk_size_status(header *hdr, size_t size, bool status)
{
	hdr->size = size;
	hdr->status = status;
}

//function from naive which only resets the status given the header pointer
void set_chunk_status(header *hdr, bool status)
{
	hdr->status = status;
}

//function from naive which will return the status of a chunk given the header(useful for checks)
bool get_chunk_status(header *hdr)
{
	return hdr->status;
}
//fucntion returns the size of a chunk given header pointer
size_t get_chunk_size(header *hdr)
{
	return hdr->size;
}

//returns a pointer that points to the very top of the header of a chunk
header *payload_to_header(void *p)
{
	return (header *)((char *)p - sizeof(header));
}

//return a pointer that points to the beginning of the payload of a chunk
void *header_to_payload(header *hdr)
{
	return (void *)((char *)hdr + sizeof(header));
}

//function not from naive, useful when looking for an appropriate sized chunk to allocate
void *header_to_next_header(header *hdr, size_t size)
{
	return (void *)((char *)hdr + size);
}

//Simple initialization for resetting a header chunk
void init_header(header *hdr) {
	hdr->next = NULL;
	hdr->prev = NULL;
	set_chunk_status(hdr, false);
}

//returns pointer to next chunk of memory, used in check_heap
header *get_next_chunk(header *hdr)
{
	size_t sz = get_chunk_size(hdr);
	return (header *)((char *)hdr + sz);
}

//function to reset pointers for either removing from the free list or the allocated list
void remove_from_list(header *hdr) {
	if (hdr == global_free_list) {
		global_free_list = hdr->next;
	}
	if (hdr == global_allocated_list_head) {
		global_allocated_list_head = hdr->next;
	}
	if (hdr->prev) {
		hdr->prev->next = hdr->next;
	}
	if (hdr->next) {
		hdr->next->prev = hdr->prev;
	}
}
int get_newsize(size_t size) {
	return align(size)+sizeof(header);
}

//function attempts to merge nxt into header
//In insert_free_list, it is shown that the free list is actually ordered in terms of address
//This that every time something is being freed, merge_regions can be called to look at both 
//the next and previous pointer to see if regions can be coalesced. Doing this every single time
//free is called ensures that the chunks in the free list remain as large as possible. In certain 
//test seeds, it can be claimed that there is too much merging going on because of the need to split
//for a small number of bytes or go straight to s_brk.
bool merge_regions(header *hdr, header *nxt) { 
	if (hdr == NULL || nxt == NULL) return false;
	if ((char *)hdr + get_chunk_size(hdr) == (char *)nxt) {
		hdr->next = nxt->next;
		if (nxt->next) nxt->next->prev = hdr;
		set_chunk_size_status(hdr, get_chunk_size(hdr) + get_chunk_size(nxt), false);
		return true;
	}
	return false;
}

//function to insert into the allocated list
void insert_allocated_list(header *hdr) {
	set_chunk_status(hdr, true);
	hdr->next = global_allocated_list_head;
	if (global_allocated_list_head) global_allocated_list_head->prev = hdr;
	hdr->prev = NULL;
	global_allocated_list_head = hdr;
	my_assert(get_chunk_status(hdr) == true);
}
//function to insert into the free list
//traverses the list to order nodes in ascending address order
//makes coalescing easier
void insert_free_list(header *hdr) {
	header* temp = global_free_list;
	set_chunk_status(hdr, false);

	if (temp == NULL) {
		global_free_list = hdr;
		hdr->prev = NULL;
		hdr->next = NULL;
		return;
	}
	if (hdr < temp) {
		hdr->next = temp;
		hdr->prev = NULL;
		temp->prev = hdr;
		global_free_list = hdr;
	}
	else {
		header* last = NULL;
		while (temp) {
			if (hdr < temp) {
				hdr->next = temp;
				temp->prev->next = hdr;
				hdr-> prev = temp->prev;
				temp->prev = hdr;
				break;
			}
			last = temp;
			temp = temp->next;
		}
		if (last != NULL && temp == NULL) {
			last->next = hdr;
			hdr->prev = last;
			hdr->next = NULL;
		}
	}
	if (merge_regions(hdr, hdr->next)) {
		merge_regions(hdr, hdr->next);
	}
	if (merge_regions(hdr->prev, hdr)) {
		merge_regions(hdr->prev, hdr);
	}
	my_assert(get_chunk_status(hdr) == false);
}

//Initializes the free list and allocated list
//returns 0 like the naive implementation
int mm_init(void)
{
	global_free_list = NULL;
	global_allocated_list_head= NULL;
	return 0;
}

//Used in malloc, this attempts to find an already existing chunk in the free list
//to accomodate the given size.
void *find_memory(size_t size) {
	if (size == 0) return NULL;
	int newsize = get_newsize(size);

    for (int i = 0; i < 2; i++) {
		header *temp = global_free_list;

		while (temp) {
			// printf("Checking temp %lu status %d newsize %d ", get_chunk_size(temp), get_chunk_status(temp), newsize);

			my_assert(get_chunk_status(temp) == false);

			if (get_chunk_status(temp) == false) {
				size_t chunk_size = get_chunk_size(temp);
				if (i == 0) {
					if (chunk_size >= newsize && chunk_size < newsize + MIN_GUARD) {
						remove_from_list(temp);
						insert_allocated_list(temp);
						
						return header_to_payload(temp);
					}
				} else {
					if (chunk_size > newsize + sizeof(header)) {
						size_t orig_size = chunk_size;
						my_assert(is_aligned((void *)temp));
						set_chunk_size_status(temp, newsize, true);
						my_assert(get_chunk_status(temp) == true);
						my_assert(get_chunk_size(temp) == newsize);
						// Update global allocated list
						remove_from_list(temp);
						insert_allocated_list(temp);

						//Need to look at the newsize here.
						header *next_header = header_to_next_header(temp, newsize);
						init_header(next_header);
						my_assert(orig_size - newsize > 0);
						set_chunk_size_status(next_header, orig_size-newsize, false);
						// Update global free list
						insert_free_list(next_header);
						return header_to_payload(temp);
					}
				}

			}
			temp = temp->next;
		}
	}
	return NULL;
}


/* 
 * mm_malloc allocates a memory block of size bytes
 * First, memory is searched for in the free list. 
 * If that is not found, then mem_sbrk is called to get more memory.
 */
void *mm_malloc(size_t size)
{
	if (size == 0) return NULL;

	void *allocated_memory = find_memory(size);
	if (allocated_memory != NULL) {
		return allocated_memory;
	}

	// printf("Allocating: %ld", size);
	int newsize = align(size) + sizeof(header);

	header *h = (header *)mem_sbrk(newsize);
	// if (h == (void *)-1)
	// 	return NULL;
	//else {
		my_assert(is_aligned((void *)h));
		set_chunk_size_status(h, newsize, true);
		my_assert(get_chunk_status(h) == true);
		my_assert(get_chunk_size(h) == newsize);
		// Update global allocated list
		insert_allocated_list(h);
		return header_to_payload(h);
	//}
	
	return NULL;
}



/*
 * mm_free frees the previously allocated memory block
 * asserts ensures that the pointer is not outside of the heap.
 * function effectively removes allocated memory from allocates list to free list.
 */
void mm_free(void *ptr)
{
	my_assert(ptr >= mem_heap_lo() && ptr <= mem_heap_hi());
	header *h;
	h = payload_to_header(ptr);
	// Update global free list
	remove_from_list(h);
	insert_free_list(h);
}	

/*
 * mm_realloc changes the size of the memory block pointed to by ptr to size bytes.  
 * The purpose of realloc is to efficiently move a payload into a larger sized chunk of memory.
 * Realloc calls malloc, copies payload over, the frees the given pointer because the new one
 * has been allocated. 
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *newptr = NULL;
	if (size > 0) {
		newptr = mm_malloc(size);
		// if (newptr == NULL)
		// 	return NULL;

		if (ptr != NULL) {
			header *h = payload_to_header(ptr); 
			size_t copySize = get_chunk_size(h) - sizeof(header); //payload size
			//printf("Copy size: %lu\n", copySize);

			if (size < copySize) {
				//copySize is the minimum of new payload size and old payload size
				copySize = size;
			}
			memcpy(newptr, ptr, copySize); //copy the old payload into its new location
		}
	}
	if (ptr != NULL) {
	    mm_free(ptr); 
	}
	return newptr;
}


/*
 * mm_checkheap checks the integrity of the heap and helps with debugging
 * Naive implementation of checkheap greatly helped in ensuring that pointer arithmetic was correct
 * 
 */
void mm_checkheap(int verbose_level) 
{
	// Your code here
	size_t total_allocated = 0, total_free = 0;
	size_t total_allocated_sz = 0, total_free_sz = 0;
	header *h;
	h = mem_heap_lo();
	// do a simple check by traversing all the chunks and print out their size and status
	while ((void *)h < mem_heap_hi()) {
		size_t x = mem_heapsize();
		printf("%zx\n", x);
		printf("%d %lu %lu\n", get_newsize(h->size), (unsigned long) mem_heap_lo(), 
		(unsigned long) mem_heap_hi());
		my_assert(h->next == header_to_next_header(h,h->next->size));

		if (verbose_level > NORMAL_VERBOSE) {
		       	printf("chunk size %ld status %d\n", get_chunk_size(h), get_chunk_status(h));
		}
		if (get_chunk_status(h)) {
			total_allocated_sz += get_chunk_size(h);
			total_allocated++;
		} else {
			total_free_sz += get_chunk_size(h);
			total_free++;
		}
	       	h = get_next_chunk(h);
	}
	if (verbose_level > 0) {
	       	printf("total non-free chunks %ld size %ld, total free chunks %ld size %ld\n", total_allocated, total_allocated_sz, total_free, total_free_sz);
	}
	return;
}

