#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void inicializaCadena(char *cadena, int n){
    int i;
    for(i=0; i<n/2; i++){
        cadena[i] = 'A';
    }
    for(i=n/2; i<3*n/4; i++){
        cadena[i] = 'C';
    }
    for(i=3*n/4; i<9*n/10; i++){
        cadena[i] = 'G';
    }
    for(i=9*n/10; i<n; i++){
        cadena[i] = 'T';
    }
}

int main(int argc, char *argv[])
{
    if(argc != 3){
        printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1); 
    }

    int i, n, count=0, add;
    int numProcs, rank;
    char *cadena;
    char L;

    n = atoi(argv[1]);
    L = *argv[2];

    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);

    // reparto cícilico
    // el proceso 0 hace el 0 4 8 12 16... (si fuesen 4 procesos)
    // el proceso 1 hace el 1 5 9 13 17... (si fuesen 4 procesos)

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    for(i=rank; i<n; i+=numProcs)
        if(cadena[i] == L)
            count++;

    printf("El proceso %d ha contado %d letras\n", rank, count);

    if(rank != 0)
    {
        // data, tam_data, type, rank_dest, tag, comm
        MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }
    if(rank == 0)
    {
        for(i = 1; i < numProcs; i++)
        {
            MPI_Recv(&add, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            count += add;
        }
    }

    MPI_Finalize();

    if(rank == 0)
        printf("El numero de apareciones de la letra %c es %d\n", L, count);

    free(cadena);
    exit(0);
}
