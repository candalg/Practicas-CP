#ifndef __SEM_H__
#define __SEM_H__

#include <pthread.h>

struct sem_t {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int value;
};

typedef struct sem_t sem_t;

int sem_init(sem_t *s, int value);
int sem_destroy(sem_t *s);

int sem_p(sem_t *s);
int sem_v(sem_t *s);
int sem_tryp(sem_t *s);

#endif