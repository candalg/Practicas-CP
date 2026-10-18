

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>

#include "options.h"
#include "sem.h"

// Tamaño de la sala de espera (en las diapositivas: MAX_CUSTOMERS)
#ifndef MAX_WAITING_SEATS
#define MAX_WAITING_SEATS 10
#endif

static sem_t customers;   // nº de clientes esperando
static sem_t barber;      // nº de barberos disponibles
static sem_t free_seats;  // nº de asientos libres en sala de espera

static struct options opt;

static atomic_int done = 0;

static void *barber_thread(void *arg)
{
    long id = (long)(intptr_t)arg;

    while (1) {
        sem_p(&customers);      // Esperar clientes

        if (atomic_load(&done))
            break;

        sem_v(&barber);         // Llamar a un cliente

        printf("Barbero %ld: cortando el pelo\n", id);
        usleep(opt.cut_time);
    }

    return NULL;
}

static void *customer_thread(void *arg)
{
    long id = (long)(intptr_t)arg;

    // Intentar reservar silla
    if (sem_tryp(&free_seats) != 0) {
        printf("Cliente %ld: sala llena, me marcho\n", id);
        return NULL;
    }

    printf("Cliente %ld: esperando en la sala\n", id);

    sem_v(&customers);      // Avisar que hay cliente
    sem_p(&barber);         // Esperar a que un barbero lo llame

    sem_v(&free_seats);     // Liberar silla de espera

    printf("Cliente %ld: me están cortando el pelo\n", id);

    return NULL;
}

int main(int argc, char **argv)
{
    opt.barbers = 5;
    opt.customers = 1000;
    opt.cut_time = 3000;

    read_options(argc, argv, &opt);

    sem_init(&customers, 0);
    sem_init(&barber, 0);
    sem_init(&free_seats, MAX_WAITING_SEATS);

    pthread_t *barbers = calloc(opt.barbers, sizeof(pthread_t));
    pthread_t *customers_th = calloc(opt.customers, sizeof(pthread_t));

    if (!barbers || !customers_th) {
        perror("calloc");
        exit(1);
    }

    for (long i = 0; i < opt.barbers; i++) {
        if (pthread_create(&barbers[i], NULL, barber_thread,
                           (void *)(intptr_t)i) != 0) {
            perror("pthread_create barber");
            exit(1);
        }
    }

    for (long i = 0; i < opt.customers; i++) {
        if (pthread_create(&customers_th[i], NULL, customer_thread,
                           (void *)(intptr_t)i) != 0) {
            perror("pthread_create customer");
            exit(1);
        }
    }

    for (int i = 0; i < opt.customers; i++)
        pthread_join(customers_th[i], NULL);

    atomic_store(&done, 1);

    // Despertar barberos para que puedan salir
    for (int i = 0; i < opt.barbers; i++)
        sem_v(&customers);

    for (int i = 0; i < opt.barbers; i++)
        pthread_join(barbers[i], NULL);

    sem_destroy(&customers);
    sem_destroy(&barber);
    sem_destroy(&free_seats);

    free(barbers);
    free(customers_th);

    return 0;
}
