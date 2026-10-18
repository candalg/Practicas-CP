#include "sem.h"

int sem_init(sem_t *s, int value) {
    if (value < 0) return -1;
    
    if (pthread_mutex_init(&s->mutex, NULL) != 0) return -1; //inicializa mutex interno del semaforo
    if (pthread_cond_init(&s->cond, NULL) != 0) { //inicializa variables de condición de semaforo
        pthread_mutex_destroy(&s->mutex); //si falla destruye mutex semaforo
        return -1;
    }
    
    s->value = value; //asigna el valor al mutex del semaforo
    return 0;
}

int sem_destroy(sem_t *s) {
    pthread_mutex_lock(&s->mutex); //bloquea semaforo
    //Se bloquea el mutex para entrar en sección crítica antes de destruir el semáforo y evitar que otro hilo esté accediendo a su estado al mismo tiempo
    pthread_mutex_unlock(&s->mutex); //lo desbloquea
    
    int s1 = pthread_cond_destroy(&s->cond); //destruye la condicion
    int s2 = pthread_mutex_destroy(&s->mutex); //destruye el mutex
    return (s1 == 0 && s2 == 0) ? 0 : -1;
}

int sem_p(sem_t *s) { // WAIT() //quiero consumir una unidad del semaforo
    pthread_mutex_lock(&s->mutex); //bloquea el mutex semaforo
    
    // Mientras no queden 'tickets' en el contador, dormimos
    while (s->value <= 0) { //si no hay unidades disponibles
        pthread_cond_wait(&s->cond, &s->mutex); //hilo se duerme y libera mutex
    }
    s->value--; // Consumir 1 unidad
    
    pthread_mutex_unlock(&s->mutex); //desbloquea mutex
    return 0;
}

int sem_v(sem_t *s) { // POST/SIGNAL() //liberar unidad o añadir ticket
    pthread_mutex_lock(&s->mutex); //bloquea mutex
    
    s->value++; // Liberar 1 unidad
    pthread_cond_signal(&s->cond); // Despertar a algún atascado en sem_p() si lo hay
    
    pthread_mutex_unlock(&s->mutex); //desbloquea mutex
    return 0;
}

int sem_tryp(sem_t *s) { // No bloqueante //intenta hacer lo mismo que sem_p pero sin bloquearse
    pthread_mutex_lock(&s->mutex); //bloquea mutex
    
    if (s->value <= 0) { //si no hay unidades disponibles
        pthread_mutex_unlock(&s->mutex); //libera mutex y falla
        return -1;
    }
    
    s->value--; //consume una unidad
    pthread_mutex_unlock(&s->mutex); //desbloquea mutex
    return 0;
}
