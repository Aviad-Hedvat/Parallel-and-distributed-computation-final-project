#include <stdio.h>
#include <stdlib.h>
#include "proto.h"
#include <time.h>
#include <omp.h>

FILE* readFile(double* value)
{
    FILE* fp;
    if ((fp = fopen(INPUT_NAME, "r")) == 0)
    {
        printf("cannot open file %s for reading\n", INPUT_NAME);
        exit(0);
    }
    fscanf(fp, "%lf\n", value);
    return fp;
}

object* readMatrices(FILE* fp, int* size, int toClose)
{
    object* matrices;
    fscanf(fp, "%d\n", size);
    matrices = (object*)calloc((*size), sizeof(object));
    if (!matrices)
    {
        printf("problem in memory allocation!\n");
        exit(0);
        return NULL;
    }
    
    for (int i = 0; i < *size; i++)
    {
        matrices[i] = readMat(fp);
    }
    if (toClose)
    {
        fclose(fp);
    }
    return matrices;

}

object readMat(FILE* fp)
{
    object mat;
    fscanf(fp, "%d\n", &mat.id);
    fscanf(fp, "%d\n", &mat.dim);
    mat.members = (int**)calloc(mat.dim, sizeof(int*));
    for (int j = 0; j < mat.dim; j++)
    {
        mat.members[j] = (int*)calloc(mat.dim, sizeof(int));
        for (int k = 0; k < mat.dim - 1; k++)
        {
            fscanf(fp, "%d ", &mat.members[j][k]);
        }
        fscanf(fp, "%d\n", &mat.members[j][mat.dim - 1]);
    }
    return mat;
}

int matching(object picture, int row, int col, object obj, double value)
{
    double diff = 0.0;
    for (int i = 0; i < obj.dim; i++)
    {
        for (int j = 0; j < obj.dim; j++)
        {
            double p = (double)picture.members[row + i][col + j];
            double o = (double)obj.members[i][j];
            double res = myAbs((p - o) / p);
            diff += res;
            if (diff > value)
            {
                return 0;
            }
        }
    }
    return 1;
}

double myAbs(double num)
{
    if (num < 0)
    {
        return num * -1;
    }
    return num;
}

int main(int argc, char* argv[])
{
    clock_t start = clock();
    int N, K;
    double value;
    object *pictures = NULL, *objects = NULL;
    
    FILE* rf = readFile(&value);
    pictures = readMatrices(rf, &N, 0);
    objects = readMatrices(rf, &K, 1);
    int anyFound = 0, nextPic = 0;
    double diff = 0.0;
    FILE* wf = fopen(OUTPUT_NAME, "w");
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < K; j++)
        {
            if (nextPic)
            {
                break;
            }
            for (int k = 0; k < pictures[i].dim; k++)
            {
                if (nextPic)
                {
                    break;
                }
                for (int w = 0; w < pictures[i].dim; w++)
                {
                    if (k + objects[j].dim < pictures[i].dim && w + objects[j].dim < pictures[i].dim)
                    {
                        if (matching(pictures[i], k, w, objects[j], value))
                        {
                            fprintf(wf, "Picture %d found Object %d in Position(%d, %d)\n", pictures[i].id, objects[j].id, k, w);
                            anyFound = 1;
                            nextPic = 1;
                            break;
                        }
                    }
                }
            }
        }
        if (!anyFound)
        {
            fprintf(wf, "Picture %d No Objects were found\n", pictures[i].id);
        }
        anyFound = 0;
        nextPic = 0;
    }
    fclose(wf);
    clock_t end = clock();
    double sec = (double)((end - start) / CLOCKS_PER_SEC);
    printf("Execution time: %lfs\n", sec);
}