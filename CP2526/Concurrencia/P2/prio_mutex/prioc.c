#include "prio_mutex.h"
#include <stdlib.h>

int prio_mutex_init(prio_mutex_t *m, int num_prios) {
    if (num_prios <= 0) return -1; //numero de prioridad valido
    
    if (pthread_mutex_init(&m->state_mutex, NULL) != 0) //inicializa el de estado
        return -1;
        
    m->waiters = (int*) malloc(num_prios * sizeof(int)); //se reserva memoria para el array waiters
    if (!m->waiters) { //comprueba si la reserva fallo
        pthread_mutex_destroy(&m->state_mutex); //si fallo se destruye el de estado
        return -1;
    }
    
    m->conds = (pthread_cond_t*) malloc(num_prios * sizeof(pthread_cond_t)); //se reserva memoria para el array de variables de condicion
    if (!m->conds) { // si falla
        free(m->waiters); //se libera waiters
        pthread_mutex_destroy(&m->state_mutex); //se destruye el mutex de estado
        return -1;
    }
    
    for (int i = 0; i < num_prios; i++) { //bucle que recorre todas las prioridades
        m->waiters[i] = 0; //se pone a 0 el numero de hilos esperando en esa prioridad
        if (pthread_cond_init(&m->conds[i], NULL) != 0) { //se intenta inicializar la variable de condición de esa prioridad
            // Manejador de error si falla la inicializacion de cond_vars (hacer rollback)
            for(int j = 0; j < i; j++) pthread_cond_destroy(&m->conds[j]); //si falla la condición i se destruyen todas als condiciones creadas anteriormente
            free(m->conds); //se libera el array de condiciones
            free(m->waiters); //el de waiters
            pthread_mutex_destroy(&m->state_mutex); //se destruye el de estado
            return -1;
        }
    }
    
    m->locked = 0; //se indica que el mutex empieza libre
    m->num_prios = num_prios; //se guarda cuantos niveles de prioridad tiene este mutex
    return 0;
}

int prio_mutex_destroy(prio_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //bloquea el mutex de estado
    if (m->locked) { //si el mutex base esta bloqueado
        pthread_mutex_unlock(&m->state_mutex); //desbloquea el mutex de estado
        return -1; // Fallar si esta en uso
    }
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el mutex de estado
    
    for (int i = 0; i < m->num_prios; i++) { //se recorren las prioridades
        pthread_cond_destroy(&m->conds[i]); //se van destruyendo las condiciones
    }
    free(m->conds); //se libera array de condiciones
    free(m->waiters); //de waiters
    return pthread_mutex_destroy(&m->state_mutex); //se devuelve si se destruye el mutex de estado
}

// Devuelve 1 si hay alguien con MEJOR prioridad (número menor) esperando
static int has_higher_priority_waiters(prio_mutex_t *m, int prio) { //ayuda a decidir quien tiene preferencia
    for (int i = 0; i < prio; i++) {
        if (m->waiters[i] > 0) return 1;
    }
    return 0;
}

int prio_mutex_lock(prio_mutex_t *m, int prio) {
    if (prio < 0 || prio >= m->num_prios) return -1; //comprueba si la prioridad es valida
    
    pthread_mutex_lock(&m->state_mutex); //bloquea mutex de estado
    
    // Esperar si esta bloqueado, O SI esta libre pero hay alguien de mayor prioridad primero
    while (m->locked || has_higher_priority_waiters(m, prio)) {
        m->waiters[prio]++; //se aumenta el numero de hilos esperando
        pthread_cond_wait(&m->conds[prio], &m->state_mutex); //hace que se libere el mutex de estado y duerme al hilo conds prio
        m->waiters[prio]--; //se termina por lo que hay un hilo menos esperando
    }
    
    // Ya es mi turno, marco como bloqueado globalmente
    m->locked = 1;
    pthread_mutex_unlock(&m->state_mutex); //se libera el mutex de estado
    return 0;
}

int prio_mutex_unlock(prio_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //bloquea el mutex de estado
    
    m->locked = 0; //se desbloquea el mutex base
    
    // Buscar el hilo dormido de MAXIMA prioridad (menor índice i)
    for (int i = 0; i < m->num_prios; i++) {
        if (m->waiters[i] > 0) {
            pthread_cond_signal(&m->conds[i]); // Despertar solo a uno de esa prioridad
            break;
        }
    }
    
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el mutex de estado
    return 0;
}

int prio_mutex_trylock(prio_mutex_t *m, int prio) { 
    if (prio < 0 || prio >= m->num_prios) return -1; //si no es una prioridad valida
    
    pthread_mutex_lock(&m->state_mutex); //se bloquea le de estado
    
    if (m->locked || has_higher_priority_waiters(m, prio)) { //si esta bloqueado o hay alguien de mas prioridad primero
        pthread_mutex_unlock(&m->state_mutex); //se desbloquea el de estado
        return -1; // Retornar error porque alguien con más prioridad lo quiere o ya está usado
    }
    
    m->locked = 1; //se bloquea el base
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el de estado
    return 0;
}
