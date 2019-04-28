#ifndef HTABLE_H
#define HTABLE_H

#include "rwlock.h"

#define BIG_PRIME 1560007

//node is the type of a linked list node type. Each hash table entry corresponds to a linked list containing key/value tuples that are hashed to the same slot.
typedef struct node {
	int hashcode;
	char *key;
	void *val;
	struct node *next;
}node;

typedef struct {
	int allow_resize;
	node **store; //to contain an array of linked list heads
	int size; //size of the array
}htable;

//initialize hashtable with pointer ht, number of elements greater than sz,
//int allow_size should be 0 for no resizes and 1 if resizing necessary
void htable_init(htable *ht, int sz, int allow_resize);
//Destroys hashtable and frees up memory for other use
void htable_destroy(htable *ht);
//Returns number of elements given an ht pointer to a hashtable
int htable_size(htable *ht);
//(Exclusive mode)Inserts some value val given a pointer to a hashtable, given key associated with val
int htable_insert(htable *ht, char *key, void *val);
//(Shared mode)Looks up a val in a hashtable given pointer to table and key
void *htable_lookup(htable *ht, char *key);

#endif
