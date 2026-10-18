#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <mpi.h>

#define DEBUG 0

/* Translation of the DNA bases
   A -> 0
   C -> 1
   G -> 2
   T -> 3
   N -> 4*/

#define M  1000000 // Number of sequences
#define N  200  // Number of bases per sequence

unsigned int g_seed = 0;

int fast_rand(void) {
    g_seed = (214013*g_seed+2531011);
    return (g_seed>>16) % 5;
}

// The distance between two bases
int base_distance(int base1, int base2){
  if((base1 == 4) || (base2 == 4)){
    return 3;
  }
  if(base1 == base2) {
    return 0;
  }
  if((base1 == 0) && (base2 == 3)) {
    return 1;
  }
  if((base2 == 0) && (base1 == 3)) {
    return 1;
  }
  if((base1 == 1) && (base2 == 2)) {
    return 1;
  }
  if((base2 == 2) && (base1 == 1)) {
    return 1;
  }
  return 2;
}

int main(int argc, char *argv[]) {
    int rank, size;
    struct timeval tv1, tv2;
    double t_comm = 0.0, t_comp = 0.0;
    double start_time, end_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int *scatter_counts = malloc(size * sizeof(int));
    int *scatter_displs = malloc(size * sizeof(int));
    int *gather_counts = malloc(size * sizeof(int));
    int *gather_displs = malloc(size * sizeof(int));

    int current_scatter_displ = 0;
    int current_gather_displ = 0;
    for (int i = 0; i < size; i++) {
        int count_i = M / size + (i < (M % size) ? 1 : 0);
        gather_counts[i] = count_i;
        scatter_counts[i] = count_i * N;
        gather_displs[i] = current_gather_displ;
        scatter_displs[i] = current_scatter_displ;
        
        current_gather_displ += gather_counts[i];
        current_scatter_displ += scatter_counts[i];
    }

    int local_rows = gather_counts[rank];

    int *data1 = NULL;
    int *data2 = NULL;
    int *result = NULL;

    if (rank == 0) {
        data1 = (int *) malloc(M * N * sizeof(int));
        data2 = (int *) malloc(M * N * sizeof(int));
        result = (int *) malloc(M * sizeof(int));

        /* Initialize Matrices */
        for(int i=0;i<M;i++) {
            for(int j=0;j<N;j++) {
                /* random with 20% gap proportion */
                data1[i*N+j] = fast_rand();
                data2[i*N+j] = fast_rand();
            }
        }
    }

    int *local_data1 = (int *) malloc(local_rows * N * sizeof(int));
    int *local_data2 = (int *) malloc(local_rows * N * sizeof(int));
    int *local_result = (int *) malloc(local_rows * sizeof(int));

    if (rank == 0) {
        gettimeofday(&tv1, NULL);
    }
    
    // Distribute data
    start_time = MPI_Wtime();
    MPI_Scatterv(data1, scatter_counts, scatter_displs, MPI_INT, local_data1, local_rows * N, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatterv(data2, scatter_counts, scatter_displs, MPI_INT, local_data2, local_rows * N, MPI_INT, 0, MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    t_comm += (end_time - start_time);

    // Compute
    start_time = MPI_Wtime();
    for(int i=0; i<local_rows; i++) {
        local_result[i] = 0;
        for(int j=0; j<N; j++) {
            local_result[i] += base_distance(local_data1[i*N+j], local_data2[i*N+j]);
        }
    }
    end_time = MPI_Wtime();
    t_comp += (end_time - start_time);

    // Gather result
    start_time = MPI_Wtime();
    MPI_Gatherv(local_result, local_rows, MPI_INT, result, gather_counts, gather_displs, MPI_INT, 0, MPI_COMM_WORLD);
    end_time = MPI_Wtime();
    t_comm += (end_time - start_time);

    if (rank == 0) {
        gettimeofday(&tv2, NULL);
    }

    // Gather times
    double local_times[2] = {t_comm, t_comp};
    double *all_times = NULL;
    if (rank == 0) {
        all_times = (double *) malloc(size * 2 * sizeof(double));
    }
    MPI_Gather(local_times, 2, MPI_DOUBLE, all_times, 2, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        int microseconds = (tv2.tv_usec - tv1.tv_usec)+ 1000000 * (tv2.tv_sec - tv1.tv_sec);

        /* Display result */
        if (DEBUG == 1) {
            int checksum = 0;
            for(int i=0;i<M;i++) {
                checksum += result[i];
            }
            printf("Checksum: %d\n ", checksum);
        } else if (DEBUG == 2) {
            for(int i=0;i<M;i++) {
                printf(" %d \t ",result[i]);
            }
            printf("\n");
        } else {
            printf ("Time (seconds) = %lf\n", (double) microseconds/1E6);
            for (int i = 0; i < size; i++) {
                printf("Proceso %d: Tiempo Comunicaciones = %lf s, Tiempo Computacion = %lf s\n", i, all_times[i*2], all_times[i*2+1]);
            }
        }    

        free(data1); 
        free(data2); 
        free(result);
        free(all_times);
    }

    free(local_data1);
    free(local_data2);
    free(local_result);
    free(scatter_counts);
    free(scatter_displs);
    free(gather_counts);
    free(gather_displs);

    MPI_Finalize();
    return 0;
}
