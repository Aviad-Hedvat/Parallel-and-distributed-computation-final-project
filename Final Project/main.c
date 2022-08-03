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
    int N, K;
    double value, start;
    object *pictures = NULL, *objects = NULL;
    MPI_Status status;
    int totalLen = 0, myLen = 0, len = 0;
    
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
        start = MPI_Wtime();
        FILE* rf = readFile(&value);
        pictures = readMatrices(rf, &N, 0);
        objects = readMatrices(rf, &K, 1);
    }

    // broadcasting all common variables
    MPI_Bcast(&value, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&K, 1, MPI_INT, 0, MPI_COMM_WORLD);

    object *myPics;
    int numOfPics = N/2;
    myPics = (object*)calloc(numOfPics, sizeof(object));
    if (rank == 0)
    {
        int center = N/2;
        // in case N is odd
        if (N % 2)
        {
            center +=1;
            numOfPics += 1;
            free(myPics);
            myPics = (object*)calloc(numOfPics, sizeof(object));
        }
        // copy N/2 pictures from master to its memory  
        for (int i = 0; i < center; i++)
            copyPic(&myPics[i], &pictures[i]);
        // send N/2 pictures to other computer
        for (int i = center; i < N; i++)
        {
            MPI_Send(&pictures[i].id, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Send(&pictures[i].dim, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            for (int row = 0; row < pictures[i].dim; row++)
                MPI_Send(pictures[i].members[row], pictures[i].dim, MPI_INT, 1, 0, MPI_COMM_WORLD);
        }
        // send all objects to other computer
        for (int i = 0; i < K; i++)
        {
            MPI_Send(&objects[i].id, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Send(&objects[i].dim, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            for (int row = 0; row < objects[i].dim; row++)
                MPI_Send(objects[i].members[row], objects[i].dim, MPI_INT, 1, 0, MPI_COMM_WORLD);
        }
        
    }
    else 
    {
        // recieve N/2 pictures from master
        for (int i = 0; i < numOfPics; i++)
        {
            MPI_Recv(&myPics[i].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&myPics[i].dim, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            myPics[i].members = (int**)calloc(myPics[i].dim, sizeof(int*));
            for (int row = 0; row < myPics[i].dim; row++)
            {
                myPics[i].members[row] = (int*)calloc(myPics[i].dim, sizeof(int));
                MPI_Recv(myPics[i].members[row], myPics[i].dim, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            }
        }
        // recieve all objects from master
        objects = (object*)calloc(K, sizeof(object));
        for (int i = 0; i < K; i++)
        {
            MPI_Recv(&objects[i].id, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&objects[i].dim, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            objects[i].members = (int**)calloc(objects[i].dim, sizeof(int*));
            for (int row = 0; row < objects[i].dim; row++)
            {
                objects[i].members[row] = (int*)calloc(objects[i].dim, sizeof(int));
                MPI_Recv(objects[i].members[row], objects[i].dim, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
            }
        }
    }

    int res, x, y;
    FILE* wf = NULL;
    if (rank == 0)
        wf = fopen(OUTPUT_NAME, "w");
    else
        MPI_Send(&numOfPics, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    #pragma omp parallel for private(res, x, y)
    for (int i = 0; i < numOfPics; i++)
    {
        res = 0;
        for (int j = 0; j < K; j++)
        {
            res = findSubMat(myPics[i], objects[j], value, &x, &y);
            if (res)
            {
                if (rank == 0)
                    fprintf(wf, "Picture %d found Object %d in Position(%d, %d)\n", myPics[i].id, objects[j].id, x, y);
                else
                {
                    MPI_Send(&res, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    MPI_Send(&(myPics[i].id), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    MPI_Send(&(objects[i].id), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    MPI_Send(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                    MPI_Send(&y, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
                j = K;
            }
        }
        if (!res)
        {
            if (rank == 0)
                fprintf(wf, "Picture %d No Objects were found\n", myPics[i].id);
            else
            {
                MPI_Send(&res, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&(myPics[i].id), 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        }
    }

    if (rank == 0)
    {
        x = 0, y = 0;
        int picId, objId, pics;
        MPI_Recv(&pics, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
        for (int i = 0; i < pics; i++)
        {
            MPI_Recv(&res, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
            if (res)
            {
                MPI_Recv(&picId, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&objId, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&x, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&y, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
                fprintf(wf, "Picture %d found Object %d in Position(%d, %d)\n", picId, objId, x, y);
            }
            else
            {
                MPI_Recv(&picId, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);
                fprintf(wf, "Picture %d No Objects were found\n", picId);
            }
        }
        fclose(wf);
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