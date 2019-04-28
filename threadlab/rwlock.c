#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include "rwlock.h"

/* rwl implements a reader-writer lock.
 * A reader-write lock can be acquired in two different modes, 
 * the "read" (also referred to as "shared") mode,
 * and the "write" (also referred to as "exclusive") mode.
 * Many threads can grab the lock in the "read" mode.  
 * By contrast, if one thread has acquired the lock in "write" mode, no other 
 * threads can acquire the lock in either "read" or "write" mode.
 */

//helper function
static inline int
cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *expire)
{
	int r = 0;

	if (expire != NULL)  {
		r = pthread_cond_timedwait(c, m, expire);
	} else {
		r = pthread_cond_wait(c, m);
	}

	assert(r == 0 || r == ETIMEDOUT);
    return r;
}

//rwl_init initializes the reader-writer lock 
void
rwl_init(rwl *l)
{
	// A global lock might be needed while we initialize these variables	
	l->n_waiting_readers = 0;
	l->n_waiting_writers = 0;
	l->n_writers = 0;
	l->n_readers = 0;

	//l->mu = PTHREAD_MUTEX_INITIALIZER;
	pthread_mutex_init(&l->mu, NULL);
	//l->cond = PTHREAD_COND_INITIALIZER;
	pthread_cond_init(&l->reader_cond, NULL);
	pthread_cond_init(&l->writer_cond, NULL);
}

//rwl_nwaiters returns the number of threads *waiting* to acquire the lock
//Note: it should not include any thread who has already grabbed the lock
int
rwl_nwaiters(rwl *l) 
{
	return l->n_waiting_readers + l->n_waiting_writers;

}

//rwl_rlock attempts to grab the lock in "read" mode
//if lock is not grabbed before absolute time "expire", it returns ETIMEDOUT
//else it returns 0 (when successfully grabbing the lock)
int
rwl_rlock(rwl *l, const struct timespec *expire)
{
	pthread_mutex_lock(&l->mu);
	//printf("read lock \n");
	l->n_waiting_readers++;
	while (l->n_writers || l->n_waiting_writers) {
		int rc = cond_timedwait(&l->reader_cond, &l->mu, expire);
		//time reset every iteration
		if (rc == ETIMEDOUT) {
			//take out of waiting readers
			l->n_waiting_readers--;
			//unlock internal mutex and return ETIMEDOUT indicating failure
			pthread_mutex_unlock(&l->mu);
			return rc;
		}
	}
	//take out of waiting readers
	l->n_waiting_readers--;
	//incrememnt current readers
	l->n_readers++;
	//unlock mutex
	pthread_mutex_unlock(&l->mu);
	//return 0 for success
	return 0;
}

//rwl_runlock unlocks the lock held in the "read" mode
void
rwl_runlock(rwl *l)
{	
	//lock internal mutex
	pthread_mutex_lock(&l->mu);
	//decrement current readers
	l->n_readers--;
	//if there are waiting writers, signal one to wake up
	if (l->n_waiting_writers) {
		pthread_cond_signal(&l->writer_cond);
	} 
	//unlock internal mutex
	pthread_mutex_unlock(&l->mu);
}

//rwl_wlock attempts to grab the lock in "write" mode
//if lock is not grabbed before absolute time "expire", it returns ETIMEDOUT
//else it returns 0 (when successfully grabbing the lock)
// You can only grab the write lock after all other readers and writers have
// have completed.
int
rwl_wlock(rwl *l, const struct timespec *expire)
{
	//lock internal mutex
	assert(&l->mu);
	pthread_mutex_lock(&l->mu);
	//printf("write lock\n");
	//increment waiting writers
	l->n_waiting_writers++;
	while (l->n_writers || l->n_readers) {
		int rc = cond_timedwait(&l->writer_cond, &l->mu, expire);
		//time is reset
		if (rc == ETIMEDOUT) {
			//remove from waiting writers
			l->n_waiting_writers--;
			//unlock internal mutex
			pthread_mutex_unlock(&l->mu);
			//return ETIMEDOUT to indicate failure
			return rc;
		}
	}
	//decrement waiting writers
	l->n_waiting_writers--;
	//increment current writers
	l->n_writers++;
	//Useful assert for debugging to ensure that writers is being reset
	//properly and ensure that on a given slot there is only one writer.
	//assert(l->n_writers == 1);
	//Unlock internal mutex
	pthread_mutex_unlock(&l->mu);
	//return 0 for success
	return 0;
}

//rwl_wunlock unlocks the lock held in the "write" mode
void
rwl_wunlock(rwl *l)
{
	//lock internal mutex
	pthread_mutex_lock(&l->mu);
	//decrement current writers
	l->n_writers--;
	//Useful assert for debugging to ensure that writers is being reset
	//properly and ensure that on a given slot there are zero writers 
	//after decrementing.
	//assert(l->n_writers == 0);
	//If there are waiting writers, signal one of them(writers have priority)
	if (l->n_waiting_writers) {
		pthread_cond_signal(&l->writer_cond);
	} else if (l->n_waiting_readers) {
		//wake all waiting readers as shared mode includes all of them
		pthread_cond_broadcast(&l->reader_cond);
	}
	//unlock internal mutex
	pthread_mutex_unlock(&l->mu);
}
//rwl_destroy will take a given rwl and destroy the internal mutex
//and reader writer conditions. This is used in resize as all of 
//locks have to be destroyed when making a new table to accomodate
//for size.
void 
rwl_destroy(rwl *l)
{
	pthread_mutex_destroy(&l->mu);
	pthread_cond_destroy(&l->reader_cond);
	pthread_cond_destroy(&l->writer_cond);
}
