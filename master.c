#include "master.h"

pthread_mutex_t g_Mutex = PTHREAD_MUTEX_INITIALIZER;

//nested array to store balloon info, to figure out how to store the time
int g_balloon[5][10] = {{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0},{0,0,0,0,0,0,0,0,0,0}}; 
//0-2: values: 3:9: time

// global flag to check balloon termination
int balloon_terminate = 0;

/* This is the master io*/
int master(MPI_Comm master_comm, MPI_Comm comm)
{

    // initialisation
	int size, nslaves;
    char buf2[256], buf3[256];
    int alert_count = 0, msg_count=0;
    int header = 0;
    int check = 0;
    FILE *fp, *fp_check;

    MPI_Comm_size(master_comm, &size);
    nslaves = size - 1;
    MPI_Status status;

    omp_set_num_threads(2);

    
    // create thread for balloon sensor
    pthread_t tid;
    pthread_mutex_init(&g_Mutex, NULL);
	pthread_create(&tid, 0, ProcessFunc, NULL);

    // read status.txt and skip first line of instructions
    fp_check = fopen("status.txt", "r");
    fscanf(fp_check,"%s %s %s %s %s %s %s %s %s", 
    buf3, buf3, buf3, buf3, buf3, buf3, buf3, buf3, buf3);
    
    // check for exit status
    fscanf(fp_check,"%d", &check);
    
    // termination of sentinel
    if(check == SENTINEL ){
    
        // send termination signal to sensor node and balloon
        #pragma omp parallel for
        for (int i=0; i < nslaves; i++) {  
            sprintf(buf2, "Signal to terminate program!\n");  
            MPI_Send(buf2, strlen(buf2), MPI_CHAR, i, EXIT_TAG, master_comm);
            balloon_terminate = 1;
        }

        return 0;
    }

    fclose(fp_check);

    fp = fopen("log.txt", "a");

    //receive data from slaves
    for (int i=0; i < nslaves; i++) {  
    
        int receive_ar[12] = {0,0,0,0,0,0,0,0,0,0,0,0}; 
        char buf[256]; 

        MPI_Recv(buf, 256, MPI_CHAR, i, MPI_ANY_TAG, master_comm, &status);
        fputs(buf, stdout );
        
        msg_count++;        // count number of messages
        
        // sensor node verified eartquake
        if (status.MPI_TAG == DETECTED_TAG){
            // #pragma omp atomic
            alert_count++;  // count number of alerts
        
            if (header == 0){
                fprintf(fp, "========================================================================\n");
                fprintf(fp, "BASE STATION LOG\n");
                header = 1;
            }
            
            // base station receive earthquake readings from sensor node
            MPI_Recv(receive_ar, 11, MPI_INT, i, DETECTED_TAG, master_comm, &status);

            // compare received report with shared global array
            for( int j = 0; j < 5; j++){
                
                // calculate magnitude difference
                int magdiff = abs(receive_ar[0] - g_balloon[j][0]);
                
                // calculate distance
                int dist = distanceM(receive_ar[1], receive_ar[2], g_balloon[j][1], g_balloon[j][2], 'K'); 
                dist = abs(dist);

                // if match : conclusive event (-1), if unmatch (0)
                if (magdiff < 3 && dist < 300)                        
                {                
                    receive_ar[11] = -1;
                    break;                                  
                }                                                                                           
            }

            // write to log output file
            fprintf(fp, "\nReceived from Node %d\n", i);
            fprintf(fp, "Simulation: %d-%d-%d %d:%d:%d,",
            receive_ar[5],receive_ar[6],receive_ar[7],receive_ar[8],receive_ar[9],receive_ar[10]);
            fprintf(fp, "Magnitude: %d, Latitude: %d, Longitude: %d, Depth(km): %d\n", 
            receive_ar[0], receive_ar[1], receive_ar[2], receive_ar[4]);
            
            // write conclusive or inconclusive alert
            if (receive_ar[11] == -1){
                fprintf(fp, "Type of Alert: Conclusive Alert, ");
            }
            
            if (receive_ar[11] == 0){
                fprintf(fp, "Type of Alert: Inconclusive Alert, ");
            }
            
            fprintf(fp, "Number of Neighbours: %d, Total Messages: %d\n", receive_ar[3], msg_count);
        }  
    }
    
    
    // signal balloon for completion
    balloon_terminate  = 1;
    
    // write total number of alerts
    if (header == 1){
    fprintf(fp, "\nTotal Number of Alerts: %d\n", alert_count);
    }
    
    fprintf(fp, "\n");
    fclose(fp);
    
    // join and end thread
    pthread_join(tid, NULL);
	pthread_mutex_destroy(&g_Mutex);

    return 0;
}

/* This is the balloon sensor*/
void* ProcessFunc(void *pArg)
{
    int i =0;
    
    // loop till termination
    while (balloon_terminate == 0){
        // sleep to generate different random values for every 1 second
        sleep(1);     

        time_t t;
        t = time(NULL);
        struct tm tm = *localtime(&t);
        
        // generate readings
        int randMag = generateFloat(3,9);       // magnitude
        int randX = generateFloat(-17, -14);    // latitude
        int randY = generateFloat(164, 168);    // longitude
        int depth = generateFloat(3,7);         // depth
        
        // store readings in shared global array
        g_balloon[i%5][0] = randMag;
        g_balloon[i%5][1] = randX;
        g_balloon[i%5][2] = randY;
        g_balloon[i%5][3] = depth;
        
        // store time in shared global array
        g_balloon[i%5][4] = tm.tm_year+1900;
        g_balloon[i%5][5] = tm.tm_mon+1;
        g_balloon[i%5][6] = tm.tm_mday;
        g_balloon[i%5][7] = tm.tm_hour;
        g_balloon[i%5][8] = tm.tm_min;
        g_balloon[i%5][9] = tm.tm_sec;        
        
        i++;    // increment i
    }
    
    // display balloon array  (first in, first out)
    printf("balloon array:\n");
    for (int i=0; i<5; i++){
        printf("[Time: %d-%d-%d %d:%d:%d, Mag: %d, Lat: %d, Long: %d, Depth(km): %d]\n", 
        g_balloon[i][4], g_balloon[i][5],g_balloon[i][6],g_balloon[i][7],g_balloon[i][8],g_balloon[i][9], //yyyy-mm-dd hh:mm:ss    
        g_balloon[i][0], g_balloon[i][1], g_balloon[i][2],g_balloon[i][3]);
    }

    return 0;
}


// /*::  Official Web site: https://www.geodatasource.com                       :*/
// /*::                                                                         :*/
// /*::           GeoDataSource.com (C) All Rights Reserved 2022                :*/

double distanceM(double lat1, double lon1, double lat2, double lon2, char unit) {
  double theta, dist;
  if ((lat1 == lat2) && (lon1 == lon2)) {
    return 0;
  }
  else {
    theta = lon1 - lon2;
    dist = sin(deg2radM(lat1)) * sin(deg2radM(lat2)) + cos(deg2radM(lat1)) * cos(deg2radM(lat2)) * cos(deg2radM(theta));
    dist = acos(dist);
    dist = rad2degM(dist);
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
double deg2radM(double deg) {
  return (deg * pi / 180);
}

/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::  This function converts radians to decimal degrees             :*/
/*:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
double rad2degM(double rad) {
  return (rad * 180 / pi);
}
