#ifndef _slave_h
#define _slave_h

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
#include "master.h"

#define SHIFT_ROW 0
#define SHIFT_COL 1
#define DISP 1
#define MAGMAX 9.5                  //max magnitude
#define MAGMIN 1.0                  //max magnitude
#define K 3                         // times of iteration(detection)
#define INTERVAL 3                  // time delays in each iteration(detection)
#define THRESHOLD 2.5               //magnitude threshhold
#define pi 3.14159265358979323846

#define REQUEST_TAG 1
#define CONFIRM_TAG 2
#define REPLY_TAG 3
#define IGNORE_TAG 4
#define DETECTED_TAG 5
#define NOT_DETECTED_TAG 6
#define EXIT_TAG 7
#define SENTINEL -1

int slave(MPI_Comm master_comm, MPI_Comm comm, int dim0, int dim1);
int generateFloat(int min, int max);
double distance(double lat1, double lon1, double lat2, double lon2, char unit);
double deg2rad(double deg);
double rad2deg(double rad);

#endif
