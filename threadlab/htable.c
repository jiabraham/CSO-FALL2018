#include <stdlib.h>
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>

#include "htable.h"
#include "rwlock.h"
#include <stdbool.h>

#define MAX_COLLISION 10
#define NUM_LOCKS 10000
//static int debug = 0;
static int undergoing_resize = 0;
/* htable implements a hash table that handles collisions by chaining.
 * It contains an array, where each slot stores the head of a singly-linked list.
 * if the length of some chain is found to be longer than MAX_COLLISION after an insertion, the 
 * htable is resized by doubling the array size. 
 */
// calculate modulo addition
int
madd(int a, int b)
{
	return ((a+b)>BIG_PRIME?(a+b-BIG_PRIME):(a+b));
}

// calculate modulo multiplication
int
mmul(int a, int b)
{
	return ((a*b) % BIG_PRIME);
}


//hashcode returns the hashcode for a C string
int
hashcode(char *s) {
	int p = 0;
	int i = 0;
	while (s[i] != '\0') {
		p  = madd(mmul(p, 256), s[i]);
		i++;
	}
	return p;
}

//is_prime returns true if n is a prime number
int
is_prime(int n) {
	if (n <= 3) {
		return 1;
	}
	if ((n % 2) == 0) {
		return 0;
	}
	for (int i = 3; i < n; i+=2) {
		if ((n % i) == 0) {
			return 0;
		}
	}
	return 1;
}

//get_prime returns a prime number larger than min
int
get_prime(int min) {
	int n = min;
	while (1) {
		if (is_prime(n)) {
			return n;
		}
		n++;	
	}
}

//initialize global lock
//Even with special reader writer locks, global lock is still needed for resize
//and parts of exclusive and shared modes
pthread_mutex_t	mu = PTHREAD_MUTEX_INITIALIZER;
//initialize slot lock
rwl *rwl_slot;


//htable_init returns a new hash table that's been initialized
void
htable_init(htable *ht, int sz, int allow_resize) {
	//if (debug) printf("%d",sz);
	//initialize global lock mu
	pthread_mutex_init(&mu, NULL);
	//initialize hash table with a prime larger than sz entries
	ht->size = get_prime(sz + 1);

	//Initialized pointer to the heads of all linked lists in the table
	ht->store = (node **)malloc(sizeof(node *)*ht->size);
	//Initializes value to determine whether or not table resizing is allowed
	ht->allow_resize = allow_resize;
	assert(ht->store);
	//Initialize all entries to zero?
	bzero(ht->store, sizeof(node *)*ht->size);

	//allocate enough space for rwl's of new table
	rwl_slot = malloc(NUM_LOCKS*sizeof(rwl));
	//Initialize all rwl for each slot
	for (int i = 0; i < NUM_LOCKS; ++i) {
		rwl_init(&rwl_slot[i]);
	}
}

//htable_size returns the number of slots in the hash table
int
htable_size(htable *ht) {
	int sz = ht->size;
	return sz;
}

void
free_linked_list(node *n) {
	if (n != NULL) {
		free_linked_list(n->next);
		free(n);
	}
}

//htable_destroy destroys the htable, freeing the memory associated with its fields
void
htable_destroy(htable *ht) {
	// need to clean up every single linked list, and each of its nodes
	for (int i = 0; i < ht->size; i++)
		free_linked_list(ht->store[i]);
	free(ht->store);
	//free slot rwl's along with htable linked lists
	//cause of seg fault?
	free(rwl_slot);

}

//htable_resize increases the size of an existing htable in order to control
//max number of collsions.
static void
htable_resize(htable *ht) {
	//set undergoing_resize to true(1)
	undergoing_resize = 1;
	//set global lock
	pthread_mutex_lock(&mu);
	//if (debug) printf("old size: %d resizing htable\n",ht->size);
	//double the array size
	int new_size = get_prime(2*ht->size);
	//new pointer to array of linked list heads
	node **new_store = (node **)malloc(sizeof(node *)*new_size);
	//assert(new_store != NULL);
	//set all entries to zero
	bzero(new_store, sizeof(node *)*new_size);
	//Lock and destroy each rwlock of old htable
	//can we use htable_destroy to do this?
	for (int i = 0; i < NUM_LOCKS; ++i) {
		rwl_wlock(&rwl_slot[i], NULL);
	}
	//Copy over entries from original table to new table
	for (int i = 0; i < ht->size; i++) {
		node *curr = ht->store[i];
		while (curr) {
			node *n = curr;
			curr = curr->next;
			int slot = n->hashcode % new_size;
			//insert at the beg of new table's slot
			if (new_store[slot] != NULL) {
				n->next = new_store[slot];
			} else {
				n->next = NULL;
			}
		    new_store[slot] = n;
		}
	}
	
	//reset size to the newly updated size
	ht->size = new_size;
	//free the pointer to old destroyed linked list heads
	free(ht->store);
	//set pointer to the new linked lists heads
	ht->store = new_store;
	//Unlock global lock
	for (int i = 0; i < NUM_LOCKS; ++i) {
		rwl_wunlock(&rwl_slot[i]);
	}

	pthread_mutex_unlock(&mu);
	//set resize back to false(0)
	undergoing_resize = 0;
}

//htable_insert inserts the key, val tuple into the htable. If the key already 
//exists, it returns 1 indicating failure.  Otherwise, it inserts the new val and returns 0. 
int
htable_insert(htable *ht, char *key, void *val) {
	//if (debug) printf("allow resize %d size of  table %d \n", ht->allow_resize, ht->size);
	//Initialize boolean to check for global lock
	bool global_lock = false;
	//If another thread is resizing, global lock is needed to insert
	if (undergoing_resize) {
		pthread_mutex_lock(&mu);
		//reset global_lock because it's needed while resizing table
		global_lock = true;
	}
	//if (debug) printf("ht_insert\n");

	
	
	//calculate hash of given key
	int hcode = hashcode(key);	
	//this key/value tuple corresponds to slot "slot"
    int slot = hcode % ht->size;
    //First use writer lock because this is exclusive mode
	//DEBUGGING LOCATION OF WRITER LOCK
	rwl_wlock(&rwl_slot[slot % NUM_LOCKS], NULL);
    
	//If the boolean global_lock is true, the global lock can now
	//now be undone because the slot has been calculated and the 
	//the writer lock has been activated
    if(global_lock) {
    	pthread_mutex_unlock(&mu);
    }
    //if (debug) printf("slot created %d \n", slot);
	//traverse linked list at slot "slot", insert the new node at the end 
	node *prev = NULL; 
	node *curr = ht->store[slot];
	//if (debug) printf("prev and curr initialized\n");
	int collision = 0;
	//if (debug) printf("Entering traversal\n");
	while (curr) {
		if ((curr->hashcode == hcode ) && (strcmp(curr->key, key) == 0)) {
			rwl_wunlock(&rwl_slot[slot % NUM_LOCKS]);
			return 1; //found an existing key/value tupe with the same key
		}
		prev = curr;
		curr = curr->next;
		collision++;
	}
	//if (debug) printf("Exiting traversal\n");
	//allocate a node to store key/value tuple
	node *n = (node *)malloc(sizeof(node));
	//if (debug) printf("created new node n \n");
	//initialize all values of new node
	n->hashcode = hcode;
	n->key = key;
	n->val = val;
	n->next = NULL;
	if (prev == NULL) {
		ht->store[slot] = n;
	}else {
		prev->next = n;
	}
	//After insert writer lock can be removed
	rwl_wunlock(&rwl_slot[slot % NUM_LOCKS]);
	//check if the table needs to be resized
	if (ht->allow_resize && collision >= MAX_COLLISION) {
		htable_resize(ht);
	}
	////if (debug) printf("success \n");
	return 0; //success
}

//htable_lookup returns the corresponding val if key exists
//otherwise it returns NULL.
void *
htable_lookup(htable *ht, char *key) {
	//calculate hash value 
	int hcode = hashcode(key);
	//initialize local boolean global_lock
	bool global_lock = false;
	//check if resizing is going on 
	if (undergoing_resize) {
		//if it is, thread needs the global lock
		pthread_mutex_lock(&mu);
		//set local bool to be true
		global_lock = true;
	}
	//calculate slot with hashcode
    int slot = hcode % ht->size;
    //set reader lock because this is shared mode
	rwl_rlock(&rwl_slot[slot % NUM_LOCKS], NULL);
	//if resizing, global lock needs to be unlocked
	if (global_lock) {
		pthread_mutex_unlock(&mu);
	}
    //if (debug) printf("create slot from hash");
	//traverse linked list to view element
	node *curr = ht->store[slot];
	//if (debug) printf("current slot");
	while (curr) {
		if ((curr->hashcode == hcode) && (strcmp(curr->key, key) == 0)) {
			rwl_runlock(&rwl_slot[slot % NUM_LOCKS]);
			return curr->val;
		}	
		curr = curr->next;
	}
	//Unlock reader
	rwl_runlock(&rwl_slot[slot % NUM_LOCKS]);
	return NULL;
	
}
