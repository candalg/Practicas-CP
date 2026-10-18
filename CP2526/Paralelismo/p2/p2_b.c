#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int MPI_BinomialColectiva( // se comporta como un MPI_Bcast
  void *buffer, // direcc dato que quiero repartir
  int count, // cuantos elementos se van a enviar
  MPI_Datatype datatype, // tipo del dato
  int root, // proceso con el dato original
  MPI_Comm comm // comunicador donde participan los procesos
)
{
    int rank, numprocs, err; // id, porceso actual, total procesos, guardar codigos de error
    MPI_Comm_rank(comm, &rank); // obtiene id del proceso actual dentro del comunicador
    MPI_Comm_size(comm, &numprocs); // obtiene nº total procesos dentro de comunicador
    // Asumiendo root es 0 por simplicidad como indica la practica
    
    if(rank != root) // todos los procesos menos el root reciben datos
    {
        err = MPI_Recv(buffer, count, datatype, MPI_ANY_SOURCE, 0, comm, MPI_STATUS_IGNORE); // proceso se queda parado hasta que se le mande dato
    }

    for (int step = 1; step < numprocs; step *= 2) // recorre potencias de 2 porque en arbol binomial difusion se hace por niveles
    {
        if (rank < step) // solo procesos con rank<step envian
        {
            if ((rank + step) < numprocs) // comprueba que el destino existe
            {
                err = MPI_Send(buffer, count, datatype, rank + step, 0, comm); // proceso actual envia dato al rank + step
                if (err != MPI_SUCCESS) return err; // si falla devuelve codigo de error
            }
        }
    }
    return MPI_SUCCESS; // si sale bien devuelve exito
}

int MPI_FlattreeColectiva( // MPI_Reduce
        void *sendbuf, // dato que aporta cada proceso
        void *recvbuf, // donde ira dato final
        int count, // nº elementos
        MPI_Datatype datatype,        // ASUMIMOS MPI_INT tipo de dato
        MPI_Op op,    // ASUMIMOS QUE ES SUM tipo operacion 
        int root, // donde se guarda resultado
        MPI_Comm comm // comunicador
        )
{
    int rank, numprocs, error;
    MPI_Comm_rank(comm, &rank); 
    MPI_Comm_size(comm, &numprocs);

    int *sendptr = (int*)sendbuf; // convierto buffers void* en entero
    int *recvptr = (int*)recvbuf;

    if(rank == root) // si este proceso es el root es el que recoge
    {
        for(int i = 0; i < count; i++) // se guarda el valor local del root en recvbuf
        {
            recvptr[i] = sendptr[i];
        }
        int *temp_buffer = (int*)malloc(count * sizeof(int)); // se reserva memoria para recibir datos en cada proceso

        for(int i = 0; i < numprocs; i++) // se revisan todos los procesos
        {
            if(i != root) // se salta el root porque ya añadio su valor
            {
                error = MPI_Recv(temp_buffer, count, MPI_INT, i, 0, comm, MPI_STATUS_IGNORE); // recibe desde el proceso i en temp buffer
                if(error != MPI_SUCCESS) // si falla una recepcion
                {
                    free(temp_buffer); // se libera la memoria 
                    return error; // devuelve error
                }

                for(int j = 0; j < count; j++) // se va sumando 
                    recvptr[j] += temp_buffer[j];
            }
        }
        free(temp_buffer);
    }
    else // proceso no era el root
    {
        error = MPI_Send(sendptr, count, MPI_INT, root, 0, comm); // proceso manda dato al root
        if(error != MPI_SUCCESS) return error; // si falla envio devuelve error
    }

    return MPI_SUCCESS;
}

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
    int i, n, count=0, total_count=0;
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

    MPI_BinomialColectiva(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    cadena = (char *) malloc(n*sizeof(char));
    if(rank == 0) 
        inicializaCadena(cadena, n);
    MPI_BinomialColectiva(cadena, n, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_BinomialColectiva(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);


    for(i=rank; i<n; i+=numProcs)
        if(cadena[i] == L)
            count++;


    MPI_FlattreeColectiva(&count, &total_count, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if(rank == 0)
        printf("El numero de apareciones de la letra %c es %d\n", L, total_count);

    MPI_Finalize();
    free(cadena);
}
