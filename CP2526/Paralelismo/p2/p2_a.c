#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void inicializaCadena(char *cadena, int n){ // rellenar la cadena
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
    int i, n, count=0, total_count=0; // contador total final que usara el ultimo proceso
    int numProcs, rank;
    char *cadena;
    char L;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    if(rank == 0)
    {
        if(argc != 3){
            printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
            exit(1); 
        }
        n = atoi(argv[1]);
        L = *argv[2];
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD); // difusion colectiva. un proceso envia n los demas lo reciben. direcc dato, 1 elem, elem entero, proceso con dato original, participan todos los procesos
    cadena = (char *) malloc(n*sizeof(char)); // todos conocen n, todos reservan memoria para cadena tamaño n
    if(rank == 0) // solo proceso 0 rellena la cadena
        inicializaCadena(cadena, n);
    MPI_Bcast(cadena, n, MPI_CHAR, 0, MPI_COMM_WORLD); // todos los procesos acaban teniendo la misma cadena
    MPI_Bcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD); // todos los procesos saben que letra buscar


    for(i=rank; i<n; i+=numProcs)
        if(cadena[i] == L)
            count++;


    MPI_Reduce(&count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD); // para combinar valores de todos los procesos y obtener un resultado. dato que aporta cada proceso, donde se guardara, un elem, elem entero, operacion de suma, se guarda en proceso 0, participan todos

    if(rank == 0)
        printf("El numero de apareciones de la letra %c es %d\n", L, total_count);

    MPI_Finalize();
    free(cadena);
}
