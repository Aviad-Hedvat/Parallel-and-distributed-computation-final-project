#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "mpi.h"
#include "proto.h"

int main(int argc, char* argv[])
{
    // all relevant variables
    int rank, size;
    int N, K; // N = Number of Pictures, K = Number of Objects
    double value, start; // value = matching value, start = to measure time
    object *pictures = NULL, *objects = NULL; 
    MPI_Status status;
    
    /* Start MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // num of processes check
    if (size != 2)
    {
        printf("Program runs only with 2 processes\n");
        MPI_Abort(MPI_COMM_WORLD, 0);
    }

    // only process 0 reads the file
    if (rank == 0)
    {
        start = MPI_Wtime(); // start time measurement
        FILE* rf = readFile(&value); // open file & read matching value
        if (!rf)
        {
            printf("Problem to read input file\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        pictures = readMatrices(rf, &N, 0); // continue reading from file, read all pictures
        objects = readMatrices(rf, &K, 1); // real all objects and close file
    }

    // broadcasting all common variables
    MPI_Bcast(&value, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD); // broadcast matching value 
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD); // broadcast N
    MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD); // broadcast K

    object *myPics; // each computer will has its pictures
    int numOfPics = N/2; 
    myPics = (object*)calloc(numOfPics, sizeof(object)); // memory allocation
    if (rank == 0)
    {
        int center = N/2;
        // in case N is odd
        if (N % 2)
        {
            center +=1; // if N is odd than master will has one more picture
            numOfPics += 1; 
            free(myPics); // free memory before another allocation
            myPics = (object*)calloc(numOfPics, sizeof(object)); // memory allocation
        }
        // copy N/2 pictures from master to its memory  
        for (int i = 0; i < center; i++)
            copyPic(&myPics[i], &pictures[i]);
        // send N/2 pictures to other computer
        for (int i = center; i < N; i++)
        {
            MPI_Send(&pictures[i].id, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // send picture ID
            MPI_Send(&pictures[i].dim, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // send picture dimension
            for (int row = 0; row < pictures[i].dim; row++)
                MPI_Send(pictures[i].members[row], pictures[i].dim, MPI_INT, 1, 0, MPI_COMM_WORLD); // send n pixels of the picture
        }
        // send all objects to other computer
        for (int i = 0; i < K; i++)
        {
            MPI_Send(&objects[i].id, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // send object ID
            MPI_Send(&objects[i].dim, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // send object dimension
            for (int row = 0; row < objects[i].dim; row++)
                MPI_Send(objects[i].members[row], objects[i].dim, MPI_INT, 1, 0, MPI_COMM_WORLD); // send k pixels of the pbject
        }
        
    }
    else 
    {
        // recieve N/2 pictures from master
        for (int i = 0; i < numOfPics; i++)
        {
            MPI_Recv(&myPics[i].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // recieve picture ID
            MPI_Recv(&myPics[i].dim, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // recieve picture dimension
            myPics[i].members = (int**)calloc(myPics[i].dim, sizeof(int*)); // allocate memory for matrix pointer
            for (int row = 0; row < myPics[i].dim; row++)
            {
                myPics[i].members[row] = (int*)calloc(myPics[i].dim, sizeof(int)); // allocate memory for row of size n
                MPI_Recv(myPics[i].members[row], myPics[i].dim, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // recieve n pixels of the picture
            }
        }
        // recieve all objects from master
        objects = (object*)calloc(K, sizeof(object));
        for (int i = 0; i < K; i++)
        {
            MPI_Recv(&objects[i].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // recieve object ID
            MPI_Recv(&objects[i].dim, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // recieve object dimension
            objects[i].members = (int**)calloc(objects[i].dim, sizeof(int*)); // allocate memory for matrix pointer
            for (int row = 0; row < objects[i].dim; row++)
            {
                objects[i].members[row] = (int*)calloc(objects[i].dim, sizeof(int)); // allocate memory for row of size k
                MPI_Recv(objects[i].members[row], objects[i].dim, MPI_INT, 0, 0, MPI_COMM_WORLD, &status); // recieve n pixels of the object
            }
        }
    }

    int res, x, y; // res = represent if any object found in the picture or not, x & y will be used for the object position in the picture  
    FILE* wf = NULL; // writing file pointer
    if (rank == 0)
    {
        if (wf = fopen(OUTPUT_NAME, "w") == 0)
        {
            printf("Problem to open writing file\n");
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
    }
    else
        MPI_Send(&numOfPics, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send slave's number of pictures for future usage
    // parallelize each computer's pictures to get faster result
    // the computer will set number of threads to the computer's number of cores (probably 4 cores)
    // if the computer has less pictures than threads the computer will set the number of threads as the pictures count by default
    // otherwise the computer will share the work between all the threads
    #pragma omp parallel for private(res, x, y) 
    for (int i = 0; i < numOfPics; i++) // running over each picture the computer has
    {
        res = 0;
        for (int j = 0; j < K; j++) // running over all objects
        {
            res = findSubMat(myPics[i], objects[j], value, &x, &y); 
            if (res) // if any object found in the current picture
            {
                if (rank == 0) // the computer that writes to the output file will do so
                    fprintf(wf, "Picture %d found Object %d in Position(%d, %d)\n", myPics[i].id, objects[j].id, x, y);
                else
                {
                    // in case of the slave computer found any object in its current picture
                    // send method isn't blocking so all the results will wait for the master to finish
                    MPI_Send(&res, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send that slave do find object in the picture
                    MPI_Send(&(myPics[i].id), 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send picture ID
                    MPI_Send(&(objects[i].id), 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send object ID
                    MPI_Send(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send x position
                    MPI_Send(&y, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send y position
                }
                j = K; // in case of any foundation the computer has to continue to the next picture
            }
        }
        if (!res) // there is no object in the picture
        {
            if (rank == 0) // the computer that writes to the output file will do so
                fprintf(wf, "Picture %d No Objects were found\n", myPics[i].id);
            else
            {
                // in case that slave computer doesn't find any object in its current picture
                MPI_Send(&res, 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send that slave doesn't find object in the picture
                MPI_Send(&(myPics[i].id), 1, MPI_INT, 0, 0, MPI_COMM_WORLD); // send picture ID
            }
        }
    }

    if (rank == 0)
    {
        int picId, objId, pics; // variables that will use to recieve all data from slave computer
        MPI_Recv(&pics, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // recieve how many pictures the slave computer has
        for (int i = 0; i < pics; i++) // collect each slave pictures data
        {
            MPI_Recv(&res, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // recieve whether the slave find object or not
            if (res)
            { 
                // in case of finding recieve the relevant picture data
                MPI_Recv(&picId, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // recieve picture ID
                MPI_Recv(&objId, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // recieve object ID
                MPI_Recv(&x, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); //recieve x position
                MPI_Recv(&y, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // recieve y position
                fprintf(wf, "Picture %d found Object %d in Position(%d, %d)\n", picId, objId, x, y); // write it to the file
            }
            else
            {
                // in case of the slave doesn't find any object in the picture
                MPI_Recv(&picId, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status); // recieve picture ID
                fprintf(wf, "Picture %d No Objects were found\n", picId); // write it to the file
            }
        }
        fclose(wf); // close the file
    }

    // time measurement
    if (rank == 0)
    {
        double end = MPI_Wtime();
        printf("Execution time: %.3fs\n", (end - start));
    }

    MPI_Barrier(MPI_COMM_WORLD);
    /* shut down MPI */
    MPI_Finalize();
    return 0;
}