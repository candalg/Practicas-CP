#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "prio_mutex.h"

#define NUM_PRIOS 3
#define THREADS_PER_PRIO 2
#define TOTAL_THREADS (NUM_PRIOS * THREADS_PER_PRIO)

prio_mutex_t my_mutex;

// Variable global para comprobar el orden de entrada real
int exec_order[TOTAL_THREADS];
int exec_index = 0;
pthread_mutex_t order_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int id;
    int prio;
} thread_data_t;

void* thread_func(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    printf("Hilo %d (Prio %d) intentando lock...\n", data->id, data->prio);
    
    // Bloquear el mutex
    prio_mutex_lock(&my_mutex, data->prio);
    
    printf("Hilo %d (Prio %d) LOGRO EL LOCK.\n", data->id, data->prio);
    
    // Registrar orden
    pthread_mutex_lock(&order_mutex);
    exec_order[exec_index++] = data->prio; // Guardamos en la array de orden la prio ejecutada
    pthread_mutex_unlock(&order_mutex);
    
    // Simular un poco de trabajo critico
    usleep(50000); //Ayuda a que el lock no dure 0, y así se vea mejor el efecto de las colas y la espera de otros hilos.
    
    printf("Hilo %d (Prio %d) liberando lock...\n", data->id, data->prio);
    prio_mutex_unlock(&my_mutex);
    
    return NULL;
}

int main(void) {
    printf("Inicializando mutex de prioridad (Niveles: %d)...\n", NUM_PRIOS);
    if (prio_mutex_init(&my_mutex, NUM_PRIOS) != 0) {
        printf("Error fatal en ini!\n");
        return 1;
    }

    pthread_t threads[TOTAL_THREADS];
    thread_data_t tdata[TOTAL_THREADS];
    
    // MAIN coge el Mutex globalmente primero para hacer un "Cuello de botella"
    // Asi todos los hilos que nazcan se pondrán en las Colas de Espera
    printf("[Main] Bloqueando mutex maestro inicial, Prio 0...\n");
    prio_mutex_lock(&my_mutex, 0);
    
    // Crear y lanzar todos a la vez (mezclados para crear caos)
    int tid = 0;
    // Lanzamos primero las peores prioridades, para demostrar que los 
    // de mayor prioridad se cuelan por delante en la sala de espera.
    for (int p = NUM_PRIOS - 1; p >= 0; p--) {
        for (int i = 0; i < THREADS_PER_PRIO; i++) {
            tdata[tid].id = tid;
            tdata[tid].prio = p;
            pthread_create(&threads[tid], NULL, thread_func, &tdata[tid]);
            tid++;
        }
    }
    
    // Dejamos que todos lleguen y se atasquen en el while de wait
    sleep(1); 
    
    printf("\n[Main] Todos atascados. Liberando cerradura maestra para que entren en cascada de prioridad...\n\n");
    prio_mutex_unlock(&my_mutex);

    // Esperar al final
    for (int i = 0; i < TOTAL_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    prio_mutex_destroy(&my_mutex);
    pthread_mutex_destroy(&order_mutex);
    
    // Verificamos si realmente se cumple la propiedad
    printf("\n==== VERIFICACION DE ORDEN ESTRICTO ====\n");
    int correct = 1;
    for (int i = 0; i < TOTAL_THREADS; i++) {
        printf("Pos %d real: Ejecutó Prio %d\n", i, exec_order[i]);
        // Los primeros 2 hilos deberían ser de prio 0, luego prio 1, luego prio 2.
        int expected_prio = i / THREADS_PER_PRIO;
        if (exec_order[i] != expected_prio) {
            correct = 0;
        }
    }
    
    if (correct) {
        printf("\nRESULTADO: EXCELENTE. El orden de prioridad ha sido el estricto esperado (0->1->2).\n");
        return 0;
    } else {
        printf("\nRESULTADO: FALLO FATAL. Entraron desordenados saltandose las colas.\n");
        return 1;
    }
}
