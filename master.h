#ifndef _master_h
#define _master_h 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <omp.h>
#include <pthread.h>
// #include "slave.h"

#define DETECTED_TAG 5
#define NOT_DETECTED_TAG 6
#define EXIT_TAG 7
#define SENTINEL -1

#define MAGMAX 9.5                  //max magnitude
#define MAGMIN 1.0                  //max magnitude
#define THRESHOLD 2.5 
#define pi 3.14159265358979323846

void* ProcessFunc(void *pArg);
int master(MPI_Comm master_comm,MPI_Comm comm);
double distanceM(double lat1, double lon1, double lat2, double lon2, char unit);
double deg2radM(double deg);
double rad2degM(double rad);

#endif
