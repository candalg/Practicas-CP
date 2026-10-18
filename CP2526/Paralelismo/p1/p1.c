#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

void inicializaCadena(char *cadena, int n){ // puntero a la cadena y tamaño de ella
    int i;
    for(i=0; i<n/2; i++){ // hace que la primera mitad de la cadena sea A
        cadena[i] = 'A';
    }
    for(i=n/2; i<3*n/4; i++){ // siguiente cuarto de cadena con C
        cadena[i] = 'C';
    }
    for(i=3*n/4; i<9*n/10; i++){ // rellena con G
        cadena[i] = 'G';
    }
    for(i=9*n/10; i<n; i++){ //la parte final se ponen T
        cadena[i] = 'T';
    }
} // 50% A, 25% C, 15% G, 10% T

int main(int argc, char *argv[]) // nº argumentos que se pasan al ejecutar, vector de cadenas con argumentos
{
    if(argc != 3){ // si numero de argumentos distinto a 3 (nombre programa, n y L)
        printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1); 
    }

    int i, n, count=0, add; // cont de bucles, tamaño cadena, cont local de apariciones, variable auxiliar para recibir cantidades de otros procesos. Cada proceso su propio count
    int numProcs, rank; //nº total de procesos lanzados, id proceso actual
    char *cadena; // puntero a cadena de caracteres
    char L; // letra a contar

    n = atoi(argv[1]); // convierte texto a entero
    L = *argv[2]; // argv[2] es una cadena, por ejemplo "A", * coge el primer carácter L = A

    cadena = (char *) malloc(n*sizeof(char)); // reserva memoria para n caracteres
    inicializaCadena(cadena, n); // se llama a la funcion para llenar cadena 

    // reparto cícilico
    // el proceso 0 hace el 0 4 8 12 16... (si fuesen 4 procesos)
    // el proceso 1 hace el 1 5 9 13 17... (si fuesen 4 procesos)

    MPI_Init(&argc, &argv); // inicia entorno MPI, ahora procesos se pueden comunicar
    MPI_Comm_size(MPI_COMM_WORLD, &numProcs); // cuantos procesos hay dentro del comunicador global, se guarda en numProcs
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // que id tengo dentro del grupo, se guarda en rank

    for(i=rank; i<n; i+=numProcs) // cada proceso empieza en pos distinta y va saltando de numProcs en numProcs
        if(cadena[i] == L) // si caracter coincide con el buscado se incrementa contador
            count++;

    printf("El proceso %d ha contado %d letras\n", rank, count);

    if(rank != 0) // si el proceso no es el 0
    {
        // data, tam_data, type, rank_dest, tag, comm
        MPI_Send(&count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // el proceso envia su count al 0. Direcc mem del dato a enviar, 1 elem a enviar, elemento es entero, destino proceso 0, etiqueta de mensaje 0, comunicador
    }
    if(rank == 0) // solo proceso 0 entra aqui
    {
        for(i = 1; i < numProcs; i++) // el 0 recibe un msg de cada proceso en orden
        {
            MPI_Recv(&add, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // recibe entero desde proceso i. donde guardar lo recibido, recibe 1 elemento, tipo entero, origen msg, etiqueta esperada, comunicador, no interesa informacion extra del msg
            count += add; // el 0 suma lo que recibio del contador global
        }
    }

    MPI_Finalize(); // se cierra el entorno MPI

    if(rank == 0) // solo el 0 imprime resultado final
        printf("El numero de apareciones de la letra %c es %d\n", L, count);

    free(cadena); // se libera memoria
    exit(0);
}
