#ifndef __REC_MUTEX_H__
#define __REC_MUTEX_H__

#include <pthread.h>

struct rec_mutex_t {
    pthread_mutex_t mutex;
    pthread_mutex_t state_mutex;
    pthread_t owner;
    int lock_count;
    int waiters;

    int has_owner;
    int destroyed;
};
typedef struct rec_mutex_t rec_mutex_t;

int rec_mutex_init(rec_mutex_t *m);
int rec_mutex_destroy(rec_mutex_t *m);

int rec_mutex_lock(rec_mutex_t *m);
int rec_mutex_unlock(rec_mutex_t *m);
int rec_mutex_trylock(rec_mutex_t *m);

#endif