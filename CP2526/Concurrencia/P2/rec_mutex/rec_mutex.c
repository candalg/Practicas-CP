#include <pthread.h>
#include "rec_mutex.h"

int rec_mutex_init(rec_mutex_t *m) {

    if (pthread_mutex_init(&m->mutex, NULL) != 0)
        return -1;

    if (pthread_mutex_init(&m->state_mutex, NULL) != 0) {
        pthread_mutex_destroy(&m->mutex);
        return -1;
    }

    m->lock_count = 0;
    m->waiters = 0;
    m->has_owner = 0;
    m->destroyed = 0;
    return 0;
}

int rec_mutex_destroy(rec_mutex_t *m) {

    pthread_mutex_lock(&m->state_mutex);

    m->destroyed = 1;

    if (m->lock_count != 0 || m->waiters != 0) {
        m->destroyed = 0;
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    pthread_mutex_unlock(&m->state_mutex);

    int str1 = pthread_mutex_destroy(&m->mutex);
    int str2 = pthread_mutex_destroy(&m->state_mutex);

    return (str1 == 0 && str2 == 0) ? 0 : -1;
}

int rec_mutex_lock(rec_mutex_t *m) {
    pthread_t self = pthread_self();

    pthread_mutex_lock(&m->state_mutex);

    if (m->destroyed) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    if (m->lock_count > 0 && m->has_owner && pthread_equal(m->owner, self)) {
        m->lock_count++;
        pthread_mutex_unlock(&m->state_mutex);
        return 0;
    }

    m->waiters++;
    pthread_mutex_unlock(&m->state_mutex);

    if (pthread_mutex_lock(&m->mutex) != 0) {

        pthread_mutex_lock(&m->state_mutex);
        m->waiters--;
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    pthread_mutex_lock(&m->state_mutex);
    m->waiters--;
    m->owner = self;
    m->has_owner = 1;
    m->lock_count = 1;
    pthread_mutex_unlock(&m->state_mutex);
    return 0;
}

int rec_mutex_unlock(rec_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex);

    if (m->lock_count == 0 || !m->has_owner || !pthread_equal(m->owner, pthread_self())) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    m->lock_count--;
    int should_unlock_base = 0;

    if (m->lock_count == 0) {
        m->has_owner = 0;
        should_unlock_base = 1;
    }
    pthread_mutex_unlock(&m->state_mutex);

    if (should_unlock_base) {
        return (pthread_mutex_unlock(&m->mutex) == 0) ? 0 : -1;
    }
    return 0;
}

int rec_mutex_trylock(rec_mutex_t *m) {
    pthread_t self = pthread_self();

    pthread_mutex_lock(&m->state_mutex);

    if (m->destroyed) {
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    if (m->lock_count > 0 && m->has_owner && pthread_equal(m->owner, self)) {
        m->lock_count++;
        pthread_mutex_unlock(&m->state_mutex);
        return 0;
    }

    m->waiters++;
    pthread_mutex_unlock(&m->state_mutex);

    if (pthread_mutex_trylock(&m->mutex) != 0) {
        pthread_mutex_lock(&m->state_mutex);
        m->waiters--;
        pthread_mutex_unlock(&m->state_mutex);
        return -1;
    }

    pthread_mutex_lock(&m->state_mutex);
    m->waiters--;
    m->owner = self;
    m->has_owner = 1;
    m->lock_count = 1;
    pthread_mutex_unlock(&m->state_mutex);

    return 0;
}