#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <memory.h>
#include <omp.h>
#include <pthread.h>
#include "master.h"
#include "slave.h"

#define INTERVAL 3      // time delays in each iteration(detection)
#define K 3             // number of runs    

/* 
to run :
- make 
- make run OR mpirun -oversubscribe -np 5 main_out 2 2
- Input value in status.txt to continue (1) or terminate (-1) program
*/

int main(int argc, char **argv)
{
    // initialisation
    int rank, size;
    int dims[2] = {0,0};
    int nrows, ncols;
    
    // initialise run time
    double start, end; 
    double time_taken;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // splitting base station and wsn network
    MPI_Comm station_comm;
    MPI_Comm_split(MPI_COMM_WORLD,rank == size-1, 0, &station_comm);

    // process command line arguments
	if (argc == 3) {
		nrows = atoi (argv[1]);
		ncols = atoi (argv[2]);
		dims[0] = nrows;        // number of rows
		dims[1] = ncols;        // number of columns
		if( (nrows*ncols) != size-1) {
			printf("ERROR: nrows*ncols)=%d * %d = %d != %d\n", nrows, ncols, nrows*ncols,size);
			MPI_Finalize(); 
			return 0;
		}
		
	} else {
		nrows=ncols=(int)sqrt(size);
		dims[0]=dims[1]=0;
	}

    // executing runs
    for (int i = 0; i < K; i++) {
    
        start = MPI_Wtime();

        if (rank == size-1) 
            master(MPI_COMM_WORLD, station_comm);
        else
            slave(MPI_COMM_WORLD, station_comm, dims[0], dims[1]);

        end = MPI_Wtime();
        time_taken = end - start;
        
        printf("Run: %d , Overall time(s): %lf\n", i, time_taken);
        fflush(stdout);
        sleep(INTERVAL);
    }
    
    MPI_Comm_free(&station_comm);
    MPI_Finalize();
    return 0;
}
