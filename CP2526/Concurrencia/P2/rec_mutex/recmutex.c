#include <pthread.h>
#include "rec_mutex.h"

int rec_mutex_init(rec_mutex_t *m) { //crea mutex principal, el de estado interno, cont y banderas limpios
    if (pthread_mutex_init(&m->mutex, NULL) != 0) //intento inicializar mutex base
        return -1;
    if (pthread_mutex_init(&m->state_mutex, NULL) != 0) { //intento inicializar segundo mutex (el de info interna)
        pthread_mutex_destroy(&m->mutex); //si falla destruyes primer mutex
        return -1;
    }

    m->lock_count = 0; //nadie hizo lock todavia
    m->waiters = 0; //no hay hilos esperando por el mutex base
    m->has_owner = 0; //no hay dueño ahora
    m->destroyed = 0; //mutex no destruido (abierto y listo para usar)
    return 0;
}

int rec_mutex_destroy(rec_mutex_t *m) { //dirección del mutex a destruir
    pthread_mutex_lock(&m->state_mutex); //bloqueas el mutex de estado
    
    // Marcamos tempranamente que estamos destruyendo para evitar nuevas entradas.
    // Esto cierra la "puerta" a la diminuta carrera de lock entre el call y su state_mutex_lock
    m->destroyed = 1; //aquí decimos que el mutex no debe aceptar nuevos lock
    
    // Verificamos que no haya nadie dentro ni nadie apuntado en la cola (waiters)
    if (m->lock_count != 0 || m->waiters != 0) {
        m->destroyed = 0; // Rollback, no se pudo destruir
        pthread_mutex_unlock(&m->state_mutex); //se libera mutex de estado porque se termina de mirar
        return -1; 
    }
    pthread_mutex_unlock(&m->state_mutex); //aquí significaría que count==0 y waiters==0, se desbloquea mutex de estado
    
    int str1 = pthread_mutex_destroy(&m->mutex); //se intenta destruir el mutex base (0 si bien)
    int str2 = pthread_mutex_destroy(&m->state_mutex); //se intenta destruir mutex de estado 
    return (str1 == 0 && str2 == 0) ? 0 : -1; //si se destruyeron se devuelve 0 si no -1
}

int rec_mutex_lock(rec_mutex_t *m) { //bloquea el mutex recursivo
    pthread_t self = pthread_self(); //se guarda en self el id del hilo actual

    pthread_mutex_lock(&m->state_mutex); //se protege mutex de estado
    if (m->destroyed) { //se comprueba si mutex destruido o en proceso de, si es el caso no se puede hacer lock
        pthread_mutex_unlock(&m->state_mutex); //si es el caso se desbloquea el mutex de estado
        return -1;
    }

    // Si ya soy el dueño incremento contador
    if (m->lock_count > 0 && m->has_owner && pthread_equal(m->owner, self)) { //se comprueba si el mutex esta bloqueado por alguien, si hay un dueño valido y si el dueño soy yo (compara owner con el id guardado en self)
        m->lock_count++; //se incrementa contador
        pthread_mutex_unlock(&m->state_mutex); //se libera mutex de estado
        return 0;
    }
    
    // Si no soy el dueño, indico que voy a intentar coger el mutex base
    m->waiters++; //si el hilo actual no es el dueño indico que quiero entrar
    pthread_mutex_unlock(&m->state_mutex); //desbloqueo el mutex de estado

    // Bloqueo el mutex base (puede tardar en la cola real del SO)
    if (pthread_mutex_lock(&m->mutex) != 0) { //intento bloquear el mutex base
        pthread_mutex_lock(&m->state_mutex); //si fallo bloqueo el mutex de estado para poder modificarlo
        m->waiters--; //me quito de la cola 
        pthread_mutex_unlock(&m->state_mutex); //desbloqueo el mutex de estado
        return -1; //-1 porque fallo
    }

    // Ya lo tengo
    pthread_mutex_lock(&m->state_mutex); //si se bloquea el mutex base con éxito bloqueo el de estado
    m->waiters--; //hilo ya no esta esperando ni intentanto adquirir el mutex
    m->owner = self; //se pone como dueño al hilo actual
    m->has_owner = 1; //hay dueño valido
    m->lock_count = 1; 
    pthread_mutex_unlock(&m->state_mutex); //se libera el mutex de estado
    return 0;
}

int rec_mutex_unlock(rec_mutex_t *m) {
    pthread_mutex_lock(&m->state_mutex); //se bloquea el mutex de estado
    // Solo el dueño puede desbloquear
    if (m->lock_count == 0 || !m->has_owner || !pthread_equal(m->owner, pthread_self())) { //si mutex no bloqueado, no hay dueño valido, si hilo actual no es dueño no se puede desbloquear
        pthread_mutex_unlock(&m->state_mutex);  //si no se puede desbloquear se desbloquea el de estado
        return -1;
    }

    m->lock_count--; //se reduce el contador de locks activos
    int should_unlock_base = 0; //como mutex de estado bloqueado quiero saber si liberar mutex base

    // Solo se libera realmente cuando llega a 0
    if (m->lock_count == 0) { //si el unlock fue el ultimo
        m->has_owner = 0; // ya no hay dueño
        should_unlock_base = 1; //se marca que luego habrá que liberar el mutex base
    }
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el de estado
    
    if (should_unlock_base) { //se mira si hay que liberar mutex base, si es 1 se desbloquea
        return (pthread_mutex_unlock(&m->mutex) == 0) ? 0 : -1; 
    }
    return 0;
}

int rec_mutex_trylock(rec_mutex_t *m) {
    pthread_t self = pthread_self(); //se guarda id del hilo

    pthread_mutex_lock(&m->state_mutex); //se bloquea mutex de estado
    if (m->destroyed) { //si mutex base esta destruido o en proceso de
        pthread_mutex_unlock(&m->state_mutex); //se desbloquea el de estado
        return -1;
    }

    // Si ya soy el dueño incremento contador
    if (m->lock_count > 0 && m->has_owner && pthread_equal(m->owner, self)) {
        m->lock_count++;
        pthread_mutex_unlock(&m->state_mutex); 
        return 0;
    }
    
    // Para trylock usamos waiters fugazmente por coherencia 
    m->waiters++;
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el mutex de estado

    // Intento bloquear sin bloquearme
    if (pthread_mutex_trylock(&m->mutex) != 0) { //intenta adquirir mutex sin quedarse esperando
        pthread_mutex_lock(&m->state_mutex); //si se falla se vuelve a coger el de estado
        m->waiters--; //se corrige waiters porque ya se termino intento de entrar
        pthread_mutex_unlock(&m->state_mutex); //se desbloquea mutex de estado
        return -1;
    }

    pthread_mutex_lock(&m->state_mutex); //si se llega aquí significa que si consegui adquirir mutex base
    m->waiters--; //ya no estoy intentando entrar
    m->owner = self; //hilo actual como nuevo dueño
    m->has_owner = 1; //marcas que hay dueño valido
    m->lock_count = 1; //indicas que hay un lock activo
    pthread_mutex_unlock(&m->state_mutex); //se desbloquea el mutex de estado
    return 0;
}
