#ifndef __RW_MUTEX_H__
#define __RW_MUTEX_H__

#include <pthread.h>

struct rw_mutex_t {
    pthread_mutex_t state_mutex;
    pthread_cond_t readers_cond;
    pthread_cond_t writers_cond;

    int active_readers;
    int active_writers;
    int waiting_writers;
};

typedef struct rw_mutex_t rw_mutex_t;

int rw_mutex_init(rw_mutex_t *m);
int rw_mutex_destroy(rw_mutex_t *m);

int rw_mutex_readlock(rw_mutex_t *m);
int rw_mutex_writelock(rw_mutex_t *m);
int rw_mutex_readunlock(rw_mutex_t *m);
int rw_mutex_writeunlock(rw_mutex_t *m);

#endif