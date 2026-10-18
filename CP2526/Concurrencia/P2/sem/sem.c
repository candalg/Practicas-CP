#include "sem.h"

int sem_init(sem_t *s, int value) {
    if (value < 0) return -1;

    if (pthread_mutex_init(&s->mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&s->cond, NULL) != 0) {
        pthread_mutex_destroy(&s->mutex);
        return -1;
    }

    s->value = value;
    return 0;
}

int sem_destroy(sem_t *s) {
    pthread_mutex_lock(&s->mutex);

    pthread_mutex_unlock(&s->mutex);

    int s1 = pthread_cond_destroy(&s->cond);
    int s2 = pthread_mutex_destroy(&s->mutex);
    return (s1 == 0 && s2 == 0) ? 0 : -1;
}

int sem_p(sem_t *s) {
    pthread_mutex_lock(&s->mutex);

    while (s->value <= 0) {
        pthread_cond_wait(&s->cond, &s->mutex);
    }
    s->value--;

    pthread_mutex_unlock(&s->mutex);
    return 0;
}

int sem_v(sem_t *s) {
    pthread_mutex_lock(&s->mutex);

    s->value++;
    pthread_cond_signal(&s->cond);

    pthread_mutex_unlock(&s->mutex);
    return 0;
}

int sem_tryp(sem_t *s) {
    pthread_mutex_lock(&s->mutex);

    if (s->value <= 0) {
        pthread_mutex_unlock(&s->mutex);
        return -1;
    }

    s->value--;
    pthread_mutex_unlock(&s->mutex);
    return 0;
}