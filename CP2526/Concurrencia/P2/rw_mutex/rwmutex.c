#include "rw_mutex.h"

int rw_mutex_init(rw_mutex_t *m){
    if (pthread_mutex_init(&m->state_mutex, NULL) != 0) return -1; //inicializa mutex estado
    if (pthread_cond_init(&m->readers_cond, NULL) != 0) { //inicializa variable de condición readers
        pthread_mutex_destroy(&m->state_mutex); //si falla destruye el mutex de estado
        return -1;
    }
    if (pthread_cond_init(&m->writers_cond, NULL) != 0) { //inicializa condición de escritor
        pthread_cond_destroy(&m->readers_cond); //si falla destruye condición de lector
        pthread_mutex_destroy(&m->state_mutex); //y mutex estado
        return -1;
    }
    
    m->active_readers = 0; //pone a 0 los lectores activos
    m->active_writers = 0; //los escritores activos
    m->waiting_writers = 0; //y los escritores esperando
    return 0;
}

int rw_mutex_destroy(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //bloquea el mutex de estado
    if (m->active_readers > 0 || m->active_writers > 0) { //si hay lectores o escritor activo(s)
        pthread_mutex_unlock(&m->state_mutex); //desbloquea mutex estado y falla
        return -1; // Fallar si esta en uso
    }
    pthread_mutex_unlock(&m->state_mutex); //desnloquea mutex estado
    
    int s1 = pthread_mutex_destroy(&m->state_mutex); //destruye estado y guarda si falla o no
    int s2 = pthread_cond_destroy(&m->readers_cond); //destruye condición lector y guarda si falla
    int s3 = pthread_cond_destroy(&m->writers_cond); //destruye condición escritor y guarda si falla
    return (s1 == 0 && s2 == 0 && s3 == 0) ? 0 : -1; //si los 3 son 0 devuelve 0 (éxito)
}

int rw_mutex_readlock(rw_mutex_t *m) { //lector puede entrar si no hay escritor escribiendo ni esperando ya que tienen prioridad
    pthread_mutex_lock(&m->state_mutex); //bloquea estado
    
    // El lector debe esperar si hay algun escritor, o si hay un escritor ESPERANDO entrar
    // (para no causarle starvation)
    while (m->active_writers > 0 || m->waiting_writers > 0) { //mientras haya escritores escribiendo o esperando
        pthread_cond_wait(&m->readers_cond, &m->state_mutex); //lector se duerme esperando en readers cond y libera el estado
    }
    
    m->active_readers++; //tras esperar se aumenta numero de lectores leyendo
    
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea estado
    return 0;
}

int rw_mutex_writelock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //bloquea estado
    
    m->waiting_writers++; //aumenta numero escritores esperando
    // El escritor debe esperar a que no haya NADIE dentro (ni lectores ni escritores)
    while (m->active_writers > 0 || m->active_readers > 0) { //mientras haya escritores escribiendo o lectores leyendo
        pthread_cond_wait(&m->writers_cond, &m->state_mutex); //escritor se duerme y libera estado
    }
    m->waiting_writers--; //se disminuye numero de escritores esperando
    
    m->active_writers++; //se aumenta numero de escritores activos
    
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el de estado
    return 0;
}

int rw_mutex_readunlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //se bloquea estado
    
    m->active_readers--; //se disminuye numero de lectores activos
    
    // Si yo era el ÚLTIMO lector y hay escritores esperando, despierto a UNO de ellos
    if (m->active_readers == 0 && m->waiting_writers > 0) { //si no quedan lectores activos y hay al menos un escritor esperando
        pthread_cond_signal(&m->writers_cond); //despierta a uno
    }
    
    pthread_mutex_unlock(&m->state_mutex); //desbloquea estado
    return 0;
}

int rw_mutex_writeunlock(rw_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //bloquea estado
    
    m->active_writers--; //disminuye numero de escritores activos
    
    // Al salir un escritor, despierta al SIGUIENTE si lo hay (dar preferencia)
    // Si no hay más escritores en la cola, despierta a TODOS los lectores a la vez
    if (m->waiting_writers > 0) { //si hay escritores esperando
        pthread_cond_signal(&m->writers_cond); //despierta a uno de los escritores
    } else {
        pthread_cond_broadcast(&m->readers_cond); //despierta a todos los lectores
    }
    
    pthread_mutex_unlock(&m->state_mutex); //desbloquea estado
    return 0;
}
