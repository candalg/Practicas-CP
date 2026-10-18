#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "rec_mutex.h"

#define ARRAY_SIZE 100
#define NUM_THREADS 4
#define NUM_ITERATIONS 10000

int array[ARRAY_SIZE];
rec_mutex_t mutexes[ARRAY_SIZE];

void* thread_func(void* arg) {
    int id = *(int*)arg; //lee el entero que hay aqui
    unsigned int seed = id; //crear semilla para generador aleatorio del hilo

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int pos1 = rand_r(&seed) % ARRAY_SIZE; //generas una posición aleatoria entre 0 y 99
        int pos2 = rand_r(&seed) % ARRAY_SIZE; //segunda posición aleatoria

        if (pos1 == pos2) continue; //si las posiciones son iguales se pasa a siguiente iteracion

        int first = pos1 < pos2 ? pos1 : pos2; //se declara al primero
        int second = pos1 > pos2 ? pos1 : pos2; //se declara al segundo

        rec_mutex_lock(&mutexes[first]); //bloqueas mutex asociado al primero
        rec_mutex_lock(&mutexes[second]); //bloqueas mutex asociado al segundo

        int temp = array[first]; //guardas valor del array en variable temporal
        array[first] = array[second]; //copias valor de segunda posición en primera
        array[second] = temp; //pones valor temporal en segunda posicion

        rec_mutex_lock(&mutexes[first]); //hilo vuelve a hacer lock sobre mismo mutex

        temp = array[first];

        rec_mutex_unlock(&mutexes[first]); //desbloqueas al primero

        rec_mutex_unlock(&mutexes[second]); //desbloqueas al segundo
        rec_mutex_unlock(&mutexes[first]); //desbloqueas al primero
    }

    return NULL;
}

int main(void) {
    printf("Inicializando array y mutexes...\n");
    for (int i = 0; i < ARRAY_SIZE; i++) {
        array[i] = i;
        rec_mutex_init(&mutexes[i]);
    }

    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    printf("Lanzando %d hilos haciendo %d iteraciones cada uno...\n", NUM_THREADS, NUM_ITERATIONS);
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    long sum = 0;
    for (int i = 0; i < ARRAY_SIZE; i++) {
        sum += array[i];
        rec_mutex_destroy(&mutexes[i]);
    }

    long expected_sum = (ARRAY_SIZE - 1) * ARRAY_SIZE / 2;
    if (sum == expected_sum) {
        printf("Test PASADO correctamente. Suma final: %ld. Funcionamiento concurrente OK.\n", sum);
        return 0;
    } else {
        printf("Test FALLADO. Suma esperada %ld pero se obtuvo %ld.\n", expected_sum, sum);
        return 1;
    }
}
