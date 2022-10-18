#include "slave.h"

/* This is the slave */
int slave(MPI_Comm master_comm, MPI_Comm comm, int dim0, int dim1) //comm = station_comm
{

    int ndims=2, size, my_rank, reorder, my_cart_rank, ierr, mastersize, glb_rank;
	int nrows, ncols;
	int nbr_i_lo, nbr_i_hi;
	int nbr_j_lo, nbr_j_hi;
	MPI_Comm comm2D;
	int coord[ndims];
    int dims[2] = {dim0,dim1};
	int wrap_around[ndims];

    time_t t;
    t = time(NULL);
    struct tm tm = *localtime(&t);

    MPI_Comm_size( master_comm, &mastersize);   // size of the master communicator
    MPI_Comm_rank( master_comm, &glb_rank);     //global rank

    MPI_Comm_size( comm, &size );               //size of slave
    MPI_Comm_rank( comm, &my_rank);             //slave rank
    
    /*************************************************************/
	/* create cartesian topology for processes */
	/*************************************************************/
	MPI_Dims_create(size, ndims, dims);
	if(my_rank==0){
		printf("\nRoot Rank: %d. Comm Size: %d: Grid Dimension = [%d x %d] \n", my_rank,size,dims[0],dims[1]);
    }

	// create cartesian mapping
	wrap_around[0] = 0;
	wrap_around[1] = 0; // periodic shift is .false.
	reorder = 1;
	ierr =0;

	ierr = MPI_Cart_create(comm, ndims, dims, wrap_around, reorder, &comm2D);
	if(ierr != 0) printf("ERROR[%d] creating CART\n",ierr);

	
	/* find my coordinates in the cartesian communicator group */
	MPI_Cart_coords(comm2D, my_rank, ndims, coord); // coordinated is returned into the coord array
	/* use my cartesian coordinates to find my rank in cartesian group*/
	MPI_Cart_rank(comm2D, coord, &my_cart_rank);
	/* get my neighbors; axis is coordinate dimension of shift */
	/* axis=0 ==> shift along the rows: P[my_row-1]: P[me] : P[my_row+1] */
	/* axis=1 ==> shift along the columns P[my_col-1]: P[me] : P[my_col+1] */
	
	MPI_Cart_shift( comm2D, SHIFT_ROW, DISP, &nbr_i_lo, &nbr_i_hi );
	MPI_Cart_shift( comm2D, SHIFT_COL, DISP, &nbr_j_lo, &nbr_j_hi );
	
    MPI_Status status;
    MPI_Request request;

    int nbr_lst[4] = {nbr_i_lo, nbr_i_hi, nbr_j_lo, nbr_j_hi};  //neighbourhood
    //float recv_data[4] = {-1.0,-1.0,-1.0,-1.0};                 //neighbour values
    char nbr_pos[4][10] = {"Top", "Bottom", "Left", "Right"};
    
    // check termination message
    int flag_check = 0;
    char terminate_msg[256];
    
    MPI_Barrier(comm2D);
    MPI_Iprobe(mastersize-1, EXIT_TAG, master_comm, &flag_check, &status);
    
    if (flag_check == 1){
        MPI_Recv(terminate_msg, 256, MPI_CHAR, mastersize-1, EXIT_TAG, master_comm, &status);
        fflush(stdout);
        MPI_Comm_free(&comm2D);
        printf("Terminating!\n");
        return 0;
    }

    flag_check = 0;
    
    sleep(my_rank); //sleep to generate different random numbers
    
    // generate earthquake readings
    int randomVal = generateFloat(MAGMIN,MAGMAX);
    int randomCoorX = generateFloat(-17, -14);
    int randomCoorY = generateFloat(164, 168);
    int depth = generateFloat(3, 7);
    int event = 0;
    
    coord[0] = randomCoorX;
    coord[1] = randomCoorY;
    
    // display earthquake readings
    printf("Time:%d-%d-%d %d:%d:%d, rank: %d, randomVal: %d, lat: %d, long: %d, depth: %d\n", 
    tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900, tm.tm_hour, tm.tm_min, tm.tm_sec, my_rank, randomVal, coord[0], coord[1], depth);

    MPI_Barrier(comm2D); 
    MPI_Barrier(comm2D);
    omp_set_num_threads(2);

    if (randomVal > THRESHOLD){ // if more than threshold, send request to neighbour to get values
        int maxRecvCount=0;     // max neighbours to receive from, will reduce
        int recvCounter = 0;
        
        #pragma omp parallel for
        for (int i=0; i < 4; i++){

            if (nbr_lst[i] != MPI_PROC_NULL){
                // send to others for confirmation
                MPI_Send(&my_rank, 1, MPI_INT, nbr_lst[i], REQUEST_TAG, comm2D);
                
                // printf("H rank: %d, sends request to rank: %d\n", my_rank, nbr_lst[i] );
                // fflush(stdout);
                maxRecvCount++;
            }
        }

        int value=0, longC = 0, latC = 0,  magdiff = 0;
        int recvbuffer[3] ={0,0,0};         // [mag/rank, long., lat.]
        int sendbuffer[3] ={0,0,0};         // [mag/rank, long., lat.]

        maxRecvCount = maxRecvCount*2;
        
        // receive values from neighbours
        for (int i = 0; i < mastersize-1; i++){

            if (recvCounter < maxRecvCount){
                // printf("H rank: %d receiving...\n", my_rank);
                MPI_Recv(recvbuffer, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm2D, &status);
                // printf("H rank: %d, received: %d, from: %d, tag: %d\n", my_rank, buffer, status.MPI_SOURCE, status.MPI_TAG);
                // fflush(stdout);
                recvCounter++;
                
            }else{ // no longer receiving from neighbours
                break;
            }
            
            // if other nodes sends a request
            if (status.MPI_TAG == REQUEST_TAG){
                
                // store values to be sent
                value = recvbuffer[0];          // requester's node rank
                sendbuffer[0] = randomVal;      // magnitude
                sendbuffer[1] = coord[0];       // latitude
                sendbuffer[2] = coord[1];       // longitude
                
                //send the reply tag to rank value
                MPI_Send(sendbuffer, 3, MPI_INT, value, REPLY_TAG, comm2D);
                // printf("H rank: %d, sends: %d, to: %d, tag: \n", my_rank, randomVal, value);
                // fflush(stdout);
            }
            
            // if other node replied
            if (status.MPI_TAG == REPLY_TAG ){  
                
                // store received values
                int dist=0;
                value = recvbuffer[0];
                latC = recvbuffer[1];
                longC = recvbuffer[2];

                // printf("lat: %d, long: %d\n", coord[0], coord[1]);

                dist = distance(coord[0], coord[1], latC, longC, 'K'); //coordinate distance
                dist = abs(dist);

                // printf("value: %d, randomVal: %d\n", value, randomVal);
                magdiff = value - randomVal;
                magdiff = abs(magdiff);

                // printf("dist: %d", dist);
                // printf(" magdiff: %d\n", magdiff);
                
                //values from neighbours
                if (value > THRESHOLD){ 
                    printf("H rank: %d, received: %d, lat: %d, long: %d, from %d\n", my_rank, value, latC, longC, status.MPI_SOURCE);
                    if (magdiff < 4 && dist < 400){
                        // printf("earthquakeeee at: %d\n", my_rank);
                        event ++;   // neighbours with values more than threshold
                    }
                    
                } else{
                    printf("H rank: %d, received: %d, lat: %d, long: %d, from %d\n", my_rank, value, latC, longC, status.MPI_SOURCE);
                    // printf("H rank: %d, received: %d from %d\n", my_rank, value, status.MPI_SOURCE);
                    
                    // this neighbour won't request, reduce maximum receive count
                    maxRecvCount--; 
                }
            }


        }
    }
    else{ //BELOW threshold
    
        // if lower than threshold, listen to data and send back what you receive
        int value=0, buffer=0, flag =0;
        int recvbuffer[3] ={0,0,0};     // [mag/rank, long., lat.]
        int sendbuffer[3] ={0,0,0};     // [mag/rank, long., lat.]
        
        // continuously listens
        for (int i =0; i < mastersize-1; i++){  

            flag = 0;
            sleep(1); // explicitly sleep to give time for other nodes to send
            
            // check for available messages
            MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm2D, &flag, &status);
            
            if (flag == 1){
                // printf("L rank: %d receiving...\n", my_rank);
                // fflush(stdout);

                MPI_Recv(&buffer, 1, MPI_INT, MPI_ANY_SOURCE, REQUEST_TAG, comm2D, &status);
                // printf("L rank: %d, received: %d, from: %d, tag: %d\n", my_rank, buffer, status.MPI_SOURCE, status.MPI_TAG);
                // fflush(stdout);

                flag = 0;
            }
            
            // neighbours sends request
            if (status.MPI_TAG == REQUEST_TAG){
                value = buffer;
                sendbuffer[0] = randomVal;
                sendbuffer[1] = coord[0];
                sendbuffer[2] = coord[1];
                MPI_Send(sendbuffer, 3, MPI_INT, value, REPLY_TAG, comm2D);
                // printf("L rank: %d, sends: %d, to: %d, tag: 3\n", my_rank, randomVal, value);
                // fflush(stdout);
            } 

        }
    }
    MPI_Barrier(comm2D); 


    //send to master base station
    char buf[256];
    int sendmaster[11] = {0,0,0,0,0,0,0,0,0,0,0};
    
    // store earthquake readings
    sendmaster[0] = randomVal;
    sendmaster[1] = coord[0];
    sendmaster[2] = coord[1];
    sendmaster[3] = event;
    sendmaster[4] = depth;
    sendmaster[5] = tm.tm_year+1900;
    sendmaster[6] = tm.tm_mon+1;
    sendmaster[7] = tm.tm_mday;
    sendmaster[8] = tm.tm_hour;
    sendmaster[9] = tm.tm_min;
    sendmaster[10] = tm.tm_sec;  
    
    //sending report to master base station
    if (event >=2){ // more than 2 nodes verified
    
        // printf("sending report to master comm\n");
        // fflush(stdout);
        sprintf(buf, "Earthquake verified at %d\n", my_rank);
        MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, mastersize-1, DETECTED_TAG, master_comm );   // send message
        MPI_Send( sendmaster, 11, MPI_INT, mastersize-1, DETECTED_TAG, master_comm);           // send array

    }
    
    else{   // less than 2 nodes verified
        sprintf(buf, "No earthquake at %d\n", my_rank);
        MPI_Send( buf, strlen(buf) + 1, MPI_CHAR, mastersize-1, NOT_DETECTED_TAG, master_comm ); // send no earthquake message

    }

	MPI_Comm_free( &comm2D );
    return 0;
}

/* Function Definition */
int generateFloat(int min, int max){
	unsigned int seed = time(NULL);
    return ((max - min) * ((float)rand_r(&seed) / RAND_MAX)) + min;
}

/*::  Official Web site: https://www.geodatasource.com                       :*/
/*::                                                                         :*/
/*::           GeoDataSource.com (C) All Rights Reserved 2022                :*/

double distance(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  if ((lat1 == lat2) && (lon1 == lon2)) {
    return 0;
  }
  else {
    theta = lon1 - lon2;
    dist = sin(deg2rad(lat1)) * sin(deg2rad(lat2)) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * cos(deg2rad(theta));
    dist = acos(dist);
    dist = rad2deg(dist);
    dist = dist * 60 * 1.1515;
    switch(unit) {
      case 'M':
        break;
      case 'K':
        dist = dist * 1.609344;
        break;
      case 'N':
        dist = dist * 0.8684;
        break;
    }
    return (dist);
  }
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts decimal degrees to radians             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double deg2rad(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2deg(double rad) {
  return (rad * 180 / pi);
}
