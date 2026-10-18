#include <stdlib.h>
#include "prio_mutex.h"

int prio_mutex_init(prio_mutex_t *m, int num_prios) {
    if (num_prios <= 0) return -1;

    m->num_prios = num_prios;
    m->locked = 0;

    if (pthread_mutex_init(&m->state_mutex, NULL) != 0) return -1;

    m->waiters = (int*)malloc(num_prios * sizeof(int));
    m->conds = (pthread_cond_t*)malloc(num_prios * sizeof(pthread_cond_t));
    if (!m->waiters || !m->conds) {
        free(m->waiters); free(m->conds);
        pthread_mutex_destroy(&m->state_mutex);
        return -1;
    }

    for (int i = 0; i < num_prios; i++) {
        m->waiters[i] = 0;
        if (pthread_cond_init(&m->conds[i], NULL) != 0) {

            for(int j = 0; j < i; j++) pthread_cond_destroy(&m->conds[j]);
            free(m->waiters); free(m->conds);
            pthread_mutex_destroy(&m->state_mutex);
            return -1;
        }
    }
    return 0;
}

int prio_mutex_destroy(prio_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);
    if (m->locked) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    for (int i = 0; i < m->num_prios; i++) {
        pthread_cond_destroy(&m->conds[i]);
    }

    free(m->waiters);
    free(m->conds);
    pthread_mutex_unlock(&m->state_mutex);

    return pthread_mutex_destroy(&m->state_mutex);
}

static int has_higher_priority_waiters(prio_mutex_t *m, int my_prio) {
    for (int i = 0; i < my_prio; i++) {
        if (m->waiters[i] > 0) return 1;
    }
    return 0;
}

int prio_mutex_lock(prio_mutex_t *m, int prio) {
    if (prio < 0 || prio >= m->num_prios) return -1;

    pthread_mutex_lock(&m->state_mutex);
    m->waiters[prio]++;

    while (m->locked || has_higher_priority_waiters(m, prio)) {
        pthread_cond_wait(&m->conds[prio], &m->state_mutex);
    }

    m->waiters[prio]--;
    m->locked = 1;

    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}

int prio_mutex_unlock(prio_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);
    if (!m->locked) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    m->locked = 0;

    for (int i = 0; i < m->num_prios; i++) {
        if (m->waiters[i] > 0) {
            pthread_cond_signal(&m->conds[i]);
            break;
        }
    }

    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}

int prio_mutex_trylock(prio_mutex_t *m, int prio) {
    if (prio < 0 || prio >= m->num_prios) return -1;

    pthread_mutex_lock(&m->state_mutex);
    if (m->locked || has_higher_priority_waiters(m, prio)) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    m->locked = 1;
    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}