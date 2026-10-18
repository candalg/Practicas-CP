#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <math.h>
#include <mpi.h>

#define DEBUG 1
// EL normal da 383999162

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  1000000 // Number of sequences
#define N  200  // Number of bases per sequence

unsigned int g_seed = 0; // Variable para generar numeros aleatorios

int fast_rand(void) // funcion que devuelve numero aleatorio devuelve num entr 0 y 4
{
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5; // desplaza 16 posiciones a la derecha para quedarse con una parte y divide entre 5 para que de 0, 1, 2, 3 o 4
}

// The distance between two bases
int base_distance(int base1, int base2) 
{
    if((base1 == 4) || (base2 == 4))
        return 3;

    if(base1 == base2)
        return 0;

    if((base1 == 0) && (base2 == 3))
        return 1;

    if((base2 == 0) && (base1 == 3))
        return 1;

    if((base1 == 1) && (base2 == 2))
        return 1;

    if((base2 == 2) && (base1 == 1))
        return 1;

    return 2;
}

int main(int argc, char *argv[] )
{
    int i, j;
    int *data1, *data2; // punteros donde se guardaran las dos matrices de secuencias
    int *result; // donde se guardara el resultado final
    int numProcs, rank; // num total de procesos, id de cada uno
    struct timeval  tv1, tv2, tv_comm1, tv_comm2; // variables para medir tiempos

    MPI_Init(&argc, &argv); // se inicia el MPI
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // obtiene cuantos procesos hay en total
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // obtiene id del proceso actual

    int tamBlq = ceil((double)M / numProcs); // Truncado hacia arriba // calcula cuantas secuencias le toca a cada proceso

    // Reservar la memoria solo el proceso 0
    if(rank == 0)
    {
        data1 = (int *)     malloc(tamBlq * numProcs * N * sizeof(int)); // reserva memoria para primera matriz
        data2 = (int *)     malloc(tamBlq * numProcs * N * sizeof(int)); // reserva memoria para segunda matriz
        result = (int *)    malloc(tamBlq * numProcs * sizeof(int)); // reserva memoria para resultados
    }

    /* Initialize Matrices */
    if(rank == 0)
    {
        for(i = 0; i < M; i++) // recorre todas las secuencias
        {
            for(j = 0; j < N; j++) // recorre todas las bases de cada secuencia
            {
                /* random with 20% gap proportion */
                data1[i*N+j] = fast_rand(); // base aleatoria para cada posicion de la matriz, se guarda como array
                data2[i*N+j] = fast_rand(); //porque malloc devuelve bloque lineal de memoria no una matriz
            }
        }
    }

    int *parte_matriz1, *parte_matriz2; // cada proceso tendra una parte de  las matrices
    int *local_result; // y resultados locales

    parte_matriz1 = (int *) malloc(tamBlq * N * sizeof(int)); // reserva memoria para la parte de data1 de cada proceso
    parte_matriz2 = (int *) malloc(tamBlq * N * sizeof(int)); // para data 2
    local_result = (int *) malloc(tamBlq * sizeof(int)); // para los resultados del proceso

    // inicio tiempo de comunicacion (scatter)
    gettimeofday(&tv_comm1, NULL); 

    MPI_Scatter(data1, tamBlq * N, MPI_INT, parte_matriz1, tamBlq * N, MPI_INT, 0, MPI_COMM_WORLD); // el proceso 0 reparte data1 entre procesos
    MPI_Scatter(data2, tamBlq * N, MPI_INT, parte_matriz2, tamBlq * N, MPI_INT, 0, MPI_COMM_WORLD); // igual con data 2

    gettimeofday(&tv_comm2, NULL); // fin de comunicación
    int microseconds_scatter = (tv_comm2.tv_usec - tv_comm1.tv_usec) + 1000000 * (tv_comm2.tv_sec - tv_comm1.tv_sec); // calcula cuanto ah durado el reparto de datos

    // Aqui ajustamos por si M no es múltiplo de los procesos

    int filasValidas; // variable para saber cuantas  filas reales debe procesar cada proceso
    if(rank == numProcs - 1) // si es el ultimo proceso
        filasValidas = M - (rank * tamBlq); // calcula cuantas filas validas quedan
    else
        filasValidas = tamBlq;

    gettimeofday(&tv1, NULL); // Inicio tiempo de computación

    for(i = 0; i < tamBlq; i++) // cada proceso recorre sus filas asignadas
    {
        local_result[i] = 0; // resultado de esa fila a 0
        if(i < filasValidas) // si esa fila es valida 
        {
            for(j = 0; j < N; j++) // recorre las 200 secuencias 
            {
                local_result[i] += base_distance(parte_matriz1[i * N + j], parte_matriz2[i * N + j]); // calcula la distancia base a base y las va sumando
            }
        }
    }

    gettimeofday(&tv2, NULL);
    // fin tiempo computación
    double segundos_comp = ((tv2.tv_sec - tv1.tv_sec) + (tv2.tv_usec - tv1.tv_usec) / 1000000.0);

    gettimeofday(&tv_comm1, NULL); // inicio tiempo de comunicación (gather) 

    MPI_Gather(local_result, tamBlq, MPI_INT, result, tamBlq, MPI_INT, 0, MPI_COMM_WORLD); // cada proceso envia resultados a proceso 0

    gettimeofday(&tv_comm2, NULL);  // fin tiempo de comunicación (gather)


    int microseconds_gather = (tv_comm2.tv_usec - tv_comm1.tv_usec) + 1000000 * (tv_comm2.tv_sec - tv_comm1.tv_sec); // calcula tiempo de recogida
    double segundos_comm = (microseconds_scatter + microseconds_gather) / 1000000.0; // suma tiempos de scatter y gather

    if(rank == 0) // solo imprime proceso 0
    {
        /* Display result */
        if (DEBUG == 1) // calcula checksum
        {
            int checksum = 0;
            for(i=0;i<M;i++) { // suma todos los resultados
                checksum += result[i];
            }
            printf("Checksum: %d\n ", checksum);
        }
        else if (DEBUG == 2) // imprime todos los resultados
        {
            for(i=0;i<M;i++)
            {
                printf(" %d \t ",result[i]);
            }
        }
    }
    if(DEBUG == 0) // se imprimen tiempos
    {
        // Mandar los tiempos al proceso 0
        if(rank != 0)
        {
            MPI_Send(&segundos_comm, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
            MPI_Send(&segundos_comp, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
        }
        if(rank == 0)
        {
            // El primero ya lo puede printear
            printf("Proceso %2d | Computación: %lf s | Comunicación: %lf s\n", rank, segundos_comp, segundos_comm); // imprime sus propios tiempos
            for(i = 1; i < numProcs; i++) // recorre demas procesos
            {
                MPI_Recv(&segundos_comm, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&segundos_comp, 1, MPI_DOUBLE, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Proceso %2d | Computación: %lf s | Comunicación: %lf s\n", i, segundos_comp, segundos_comm);
            }
        }
    }

    if(rank == 0) // proceso 0 libera la memoria global
    {
        free(data1); free(data2); free(result);
    }

    free(parte_matriz1); free(parte_matriz2); free(local_result); // el resto liberan local

    MPI_Finalize();
    return 0;
}
