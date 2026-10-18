#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "rw_mutex.h"

#define NUM_READERS 5
#define NUM_WRITERS 2
#define TOTAL_THREADS (NUM_READERS + NUM_WRITERS)

rw_mutex_t rw;
int shared_data = 0; //declaro recurso compartido

int current_readers_in_cs = 0; //numero lectores dentro sc (donde se accede a shared data)
int current_writers_in_cs = 0; //igual pero para escritores
pthread_mutex_t print_mut = PTHREAD_MUTEX_INITIALIZER; //mutex normal para proteger lo anterior
int fatal_error_detected = 0; //para saber si hubo error grave

void safe_print_status(const char* msg, int id, const char* type) {
    pthread_mutex_lock(&print_mut);
    printf("[%s %d] %s (Lectores Dentro: %d | Escritores Dentro: %d | Val: %d)\n",
            type, id, msg, current_readers_in_cs, current_writers_in_cs, shared_data);

    if (current_writers_in_cs > 1 || (current_writers_in_cs > 0 && current_readers_in_cs > 0)) {
        printf("\n!!! ERROR FATAL DETECTADO: Exclusión Mutua Violada !!!\n");
        fatal_error_detected = 1;
    }
    pthread_mutex_unlock(&print_mut);
}

void* reader_func(void* arg) {
    int id = *(int*)arg;

    usleep(100000); //hace que lector espere e hilos se mezclen en el tiempo

    rw_mutex_readlock(&rw);

    pthread_mutex_lock(&print_mut);
    current_readers_in_cs++;
    pthread_mutex_unlock(&print_mut);

    safe_print_status("Entró a LECTURA", id, "Lector");

    usleep(800000); //lector se queda dentro un rato
    int local_data = shared_data; //lee valor shared y lo guarda en local
    (void)local_data;

    pthread_mutex_lock(&print_mut);
    current_readers_in_cs--;
    pthread_mutex_unlock(&print_mut);

    safe_print_status("Salió de la LECTURA", id, "Lector");

    rw_mutex_readunlock(&rw);
    return NULL;
}

void* writer_func(void* arg) {
    int id = *(int*)arg;

    usleep(200000);

    rw_mutex_writelock(&rw);

    pthread_mutex_lock(&print_mut);
    current_writers_in_cs++;
    pthread_mutex_unlock(&print_mut);

    safe_print_status("Entró a ESCRITURA exlusiva", id, "Escritor");

    shared_data += 10;
    usleep(500000);

    pthread_mutex_lock(&print_mut);
    current_writers_in_cs--;
    pthread_mutex_unlock(&print_mut);

    safe_print_status("Salió de la ESCRITURA", id, "Escritor");

    rw_mutex_writeunlock(&rw);
    return NULL;
}

int main(void) {
    printf("Inicializando mutex Read/Write...\n");
    if (rw_mutex_init(&rw) != 0) {
        printf("Error fatal en init\n");
        return 1;
    }

    pthread_t r_threads[NUM_READERS];
    pthread_t w_threads[NUM_WRITERS];
    int r_ids[NUM_READERS];
    int w_ids[NUM_WRITERS];

    printf("Lanzando hilos ( %d Lectores | %d Escritores )...\n\n", NUM_READERS, NUM_WRITERS);

    for (int i = 0; i < NUM_READERS; i++) {
        r_ids[i] = i;
        pthread_create(&r_threads[i], NULL, reader_func, &r_ids[i]);
    }

    for (int i = 0; i < NUM_WRITERS; i++) {
        w_ids[i] = i;
        pthread_create(&w_threads[i], NULL, writer_func, &w_ids[i]);
    }

    for (int i = 0; i < NUM_READERS; i++) pthread_join(r_threads[i], NULL);
    for (int i = 0; i < NUM_WRITERS; i++) pthread_join(w_threads[i], NULL);

    rw_mutex_destroy(&rw);

    printf("\n==== VALIDACION FINAL ====\n");
    if (fatal_error_detected) {
        printf("RESULTADO: FALLO GRAVE. Hubo intrusión entre un lector y un escritor a la vez.\n");
        return 1;
    } else if (shared_data != NUM_WRITERS * 10) {
         printf("RESULTADO: FALLO DE DATOS. Valor final = %d (esperado = %d)\n", shared_data, NUM_WRITERS * 10);
         return 1;
    }
    printf("RESULTADO: EXCELENTE. Múltiples lectores simultáneos, y exclusión garantizada al 100%% del Escritor.\n");

    return 0;
}
