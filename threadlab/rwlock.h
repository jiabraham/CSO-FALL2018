#ifndef RWLOCK_H
#define RWLOCK_H
//rwl struct represents reader lock applied to every slot in hashtable
typedef struct rwl {
	int n_waiting_readers;
	int n_waiting_writers;
	int n_writers;
	int n_readers;
	pthread_mutex_t mu;
	pthread_cond_t reader_cond;
	pthread_cond_t writer_cond;
} rwl;
//initializes rwl for a given rwl pointer
void rwl_init(rwl *l);
//returns total number of waiting writers and readers
int  rwl_nwaiters(rwl *l);
//reader lock which will be used for shared mode
//this lock will ensure it is only readers on shared more
int  rwl_rlock(rwl *l, const struct timespec *expire);
//reader unlock 
//will first look for waiting writer threads
void rwl_runlock(rwl *l);
//writer lock ensures only writer is current thread
int  rwl_wlock(rwl *l, const struct timespec *expire);
//writer unlock will first look for one waiting writer
//If no writers, it will broadcast to all waiting readers
void rwl_wunlock(rwl *l);
//destroy will free mutex and and two conditions internally within 
//a given rwl pointer
void rwl_destroy(rwl *l);

#endif
