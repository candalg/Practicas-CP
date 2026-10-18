#include "rw_mutex.h"

int rw_mutex_init(rw_mutex_t *m) {
    if (pthread_mutex_init(&m->state_mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&m->readers_cond, NULL) != 0) return -1;
    if (pthread_cond_init(&m->writers_cond, NULL) != 0) return -1;

    m->active_readers = 0;
    m->active_writers = 0;
    m->waiting_writers = 0;
    return 0;
}

int rw_mutex_destroy(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);
    if (m->active_readers > 0 || m->active_writers > 0) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }
    pthread_mutex_unlock(&m->state_mutex);

    pthread_cond_destroy(&m->readers_cond);
    pthread_cond_destroy(&m->writers_cond);
    return pthread_mutex_destroy(&m->state_mutex);
}

int rw_mutex_readlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);

    while (m->active_writers > 0 || m->waiting_writers > 0) {
        pthread_cond_wait(&m->readers_cond, &m->state_mutex);
    }

    m->active_readers++;
    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}

int rw_mutex_writelock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);

    m->waiting_writers++;

    while (m->active_readers > 0 || m->active_writers > 0) {
        pthread_cond_wait(&m->writers_cond, &m->state_mutex);
    }
    m->waiting_writers--;
    m->active_writers = 1;

    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}

int rw_mutex_readunlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);

    m->active_readers--;

    if (m->active_readers == 0) {
        pthread_cond_signal(&m->writers_cond);
    }

    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}

int rw_mutex_writeunlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);

    m->active_writers = 0;

    if (m->waiting_writers > 0) {
        pthread_cond_signal(&m->writers_cond);
    } else {

        pthread_cond_broadcast(&m->readers_cond);
    }

    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}