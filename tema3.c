#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "mpi.h"

#define min(a, b) (a > b ?  b : a)

const int nr_coordinators = 3;

int main (int argc, char *argv[])
{
    int  numtasks, rank;
    int i, j, x, n;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    MPI_Status status;

    int ksize = 0;
    int parent;
    int **topology = malloc(nr_coordinators * sizeof(int *));
    int *results;

    // Arbitrary max length for number of assigned workers
    for ( i = 0; i < nr_coordinators; i++) {
        topology[i] = malloc(numtasks/2 * sizeof(int));
    }

    // Current process is a coordinator
    if (rank < nr_coordinators) {
        FILE * f;

        switch (rank)
        {            
            case 0:
                f = fopen("cluster0.txt", "r");
                break;
            case 1:
                f = fopen("cluster1.txt", "r");
                break;
            case 2:
                f = fopen("cluster2.txt", "r");
                break;
            default:
                break;
        }

        ksize = fgetc(f) - 48;
        topology[rank][0] = ksize;

        for (i = 1; i <= ksize; i++){
            fgetc(f); // removing an endline character
            x = fgetc(f) - 48;

            topology[rank][i] = x;
            
            printf("M(%d,%d) ", rank, x);
            MPI_Send( &rank,  1, MPI_INT, x, 0, MPI_COMM_WORLD);
        }

        switch (rank)
        {            
            case 0:
                printf("M(%d,%d) ", rank, 2);
                MPI_Send(topology[0], numtasks/2, MPI_INT, 2, 0, MPI_COMM_WORLD);
                MPI_Recv( topology[1], numtasks/2, MPI_INT , 2, 0 , MPI_COMM_WORLD , &status);
                MPI_Recv( topology[2], numtasks/2, MPI_INT , 2 , 0 , MPI_COMM_WORLD , &status);
                break;
            case 1:
                printf("M(%d,%d) ", rank, 2);
                MPI_Send(topology[1], numtasks/2, MPI_INT, 2, 0, MPI_COMM_WORLD);
                MPI_Recv( topology[0], numtasks/2, MPI_INT , 2, 0 , MPI_COMM_WORLD , &status);
                MPI_Recv( topology[2], numtasks/2, MPI_INT , 2 , 0 , MPI_COMM_WORLD , &status);
                break;
            case 2:
                MPI_Recv( topology[1], numtasks/2, MPI_INT , 1, 0 , MPI_COMM_WORLD , &status);
                MPI_Recv( topology[0], numtasks/2, MPI_INT , 0 , 0 , MPI_COMM_WORLD , &status);
                
                printf("M(%d,%d) ", rank, 1);
                MPI_Send(topology[0], numtasks/2, MPI_INT, 1, 0, MPI_COMM_WORLD);
                MPI_Send(topology[2], numtasks/2, MPI_INT, 1, 0, MPI_COMM_WORLD);
                
                printf("M(%d,%d) ", rank, 0);
                MPI_Send(topology[1], numtasks/2, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(topology[2], numtasks/2, MPI_INT, 0, 0, MPI_COMM_WORLD);

                break;
            default:
                break;
        }

        printf("\n");
        printf("%d -> ", rank);

        // Finished discovering the topology as a coordinator and printing it
        for (i = 0; i < nr_coordinators;  i++) {
            printf("%d:", i);
            
            for (j = 1; j <= topology[i][0]; j++ ){
                printf("%d", topology[i][j]);
                
                if (j < topology[i][0]) {
                    printf(",");
                } else {
                    printf(" ");
                }
            }
        }
        printf("\n");


        // Sending the entire topology to children
        for (i = 1; i <= topology[rank][0]; i++) {
            for (j = 0; j < nr_coordinators; j++){
                printf("M(%d,%d) ", rank, topology[rank][i]);
                MPI_Send(topology[j], numtasks/2, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);
                
            }
        }

        // Distributed vector computation section
        if (rank == 0) {
            n = atoi(argv[1]);

            results = malloc((n + 1) * sizeof(int));

            for (i = 0; i < n; i++) {
                results[i] = i;
            }

            int default_worker_size = ceil((double) n / (numtasks - nr_coordinators));
            int send_size;
            int send_index = 0;

            send_size = default_worker_size;

            // Sending tasks to children
            for (i = 1; i <= topology[rank][0]; i++) {

                printf("M(%d,%d) ", rank, topology[rank][i]);
                MPI_Send(&send_size, 1, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);
                MPI_Send(results + send_index, send_size, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);

                send_index += send_size;
            }

            // Sending the rest of the vector to task #2
            send_size = n - send_index;
            printf("M(%d,%d) ", rank, 2);
            MPI_Send(&default_worker_size, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            MPI_Send(&send_size, 1, MPI_INT, 2, 0, MPI_COMM_WORLD);
            MPI_Send(results + send_index, send_size, MPI_INT, 2, 0, MPI_COMM_WORLD);       

            // reusing send variables to receive the vector back
            // starting with task 0's children
            send_index = 0;
            send_size = default_worker_size;

            for (i = 1; i <= topology[rank][0]; i++) {
                MPI_Recv( results + send_index, send_size, MPI_INT , topology[rank][i] , 0 , MPI_COMM_WORLD , &status);
                send_index += send_size;
            }

            // Receving the rest of the vector from coordinator #2
            send_size = n - send_index;
            MPI_Recv( results + send_index, send_size, MPI_INT , 2 , 0 , MPI_COMM_WORLD , &status);


        } else if (rank == 1){
            int recv_size;
            int send_index = 0;
            int send_size;
            int worker_size;


            MPI_Recv( &worker_size, 1, MPI_INT , 2, 0 , MPI_COMM_WORLD , &status);
            MPI_Recv( &recv_size, 1, MPI_INT , 2, 0 , MPI_COMM_WORLD , &status);
            results = malloc((recv_size + 1) * sizeof(int));
            MPI_Recv( results, recv_size, MPI_INT , 2 , 0 , MPI_COMM_WORLD , &status);

            send_size = worker_size;

            for (i = 1; i <= topology[rank][0]; i++) {
                if (send_index + send_size >= recv_size) {
                    send_size = recv_size - send_index;
                }

                printf("M(%d,%d) ", rank, topology[rank][i]);
                MPI_Send(&send_size, 1, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);
                MPI_Send(results + send_index, send_size, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);

                send_index += send_size;
            }

            send_index = 0;
            send_size = worker_size;
            for (i = 1; i <= topology[rank][0]; i++) {
                if (send_index + send_size >= recv_size) {
                    send_size = recv_size - send_index;
                }

                MPI_Recv( results + send_index, send_size, MPI_INT , topology[rank][i] , 0 , MPI_COMM_WORLD , &status);
                
                send_index += send_size;
            }

            printf("M(%d,%d) ", rank, 2);
            MPI_Send(results, recv_size, MPI_INT, 2, 0, MPI_COMM_WORLD);
           
        // Coordinator #2 
        } else {

            int recv_size;
            int send_index = 0;
            int send_size;
            int worker_size;


            MPI_Recv( &worker_size, 1, MPI_INT , 0, 0 , MPI_COMM_WORLD , &status);
            MPI_Recv( &recv_size, 1, MPI_INT , 0, 0 , MPI_COMM_WORLD , &status);
            results = malloc((recv_size + 1) * sizeof(int));
            MPI_Recv( results, recv_size, MPI_INT , 0 , 0 , MPI_COMM_WORLD , &status);
            
            // Sending the first portion of the vector to coordinator #1
            // based on its number of children
            send_size = worker_size * topology[1][0];
            printf("M(%d,%d) ", rank, 1);
            MPI_Send(&worker_size, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Send(&send_size, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Send(results, send_size, MPI_INT, 1, 0, MPI_COMM_WORLD);
            send_index += send_size;

            
            send_size = worker_size;
            // Sending the tasks to children
            for (i = 1; i <= topology[rank][0]; i++) {
                if (send_index + send_size >= recv_size) {
                    send_size = recv_size - send_index;
                }

                printf("M(%d,%d) ", rank, topology[rank][i]);
                MPI_Send(&send_size, 1, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);
                MPI_Send(results + send_index, send_size, MPI_INT, topology[rank][i], 0, MPI_COMM_WORLD);

                send_index += send_size;
            }

            // Receiving the results from children
            send_index = worker_size * topology[1][0];
            send_size = worker_size;
            for (i = 1; i <= topology[rank][0]; i++) {
                if (send_index + send_size >= recv_size) {
                    send_size = recv_size - send_index;
                }

                MPI_Recv( results + send_index, send_size, MPI_INT , topology[rank][i] , 0 , MPI_COMM_WORLD , &status);
                
                send_index += send_size;
            }
            
            // Receiving the first half of the vector from coordinator #1 
            MPI_Recv( results, worker_size * topology[1][0], MPI_INT , 1, 0 , MPI_COMM_WORLD , &status);
            
            printf("M(%d,%d) ", rank, 0);
            MPI_Send(results, recv_size, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }

        fclose(f);
    } else {
        // Receiving the coordinator rank
        MPI_Recv( &parent, 1 , MPI_INT , MPI_ANY_SOURCE , 0 , MPI_COMM_WORLD , &status);

        // Receiving the topoloy from the coordinator
        for (i = 0; i < nr_coordinators; i++) {
            MPI_Recv( topology[i], numtasks/2, MPI_INT , parent , 0 , MPI_COMM_WORLD , &status);
        }

        // Printing the topology received from the coordinator
        printf("\n%d -> ", rank);
        for (i = 0; i < nr_coordinators;  i++) {
            printf("%d:", i);
            
            for (j = 1; j <= topology[i][0]; j++ ){
                printf("%d", topology[i][j]);
                
                if (j < topology[i][0]) {
                    printf(",");
                } else {
                    printf(" ");
                }
            }
        }
        printf("\n");

        int recv_size;
        MPI_Recv( &recv_size, 1, MPI_INT , parent, 0 , MPI_COMM_WORLD , &status);
        results = malloc((recv_size + 1)* sizeof(int));
        MPI_Recv( results, recv_size, MPI_INT , parent , 0 , MPI_COMM_WORLD , &status);

        for (i = 0; i < recv_size; i++){
            results[i] *= 2;
        }

        printf("M(%d,%d) ", rank, parent);
        MPI_Send(results, recv_size, MPI_INT, parent, 0, MPI_COMM_WORLD);

    }


    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("\nRezultat:");

        for (i = 0; i < n; i++) {
            printf(" %d", results[i]);
        }

        printf("\n");
    }
 
    MPI_Finalize();
    return 0;
}