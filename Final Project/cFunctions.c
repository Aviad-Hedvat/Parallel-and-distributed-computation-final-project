#include <stdio.h>
#include <stdlib.h>
#include "proto.h"

// function to open the reading file and read matching value
FILE* readFile(double* value)
{
    FILE* fp;
    if ((fp = fopen(INPUT_NAME, "r")) == 0)
    {
        printf("cannot open file %s for reading\n", INPUT_NAME);
        return NULL;
    }
    fscanf(fp, "%lf\n", value);
    return fp;
}

// function to read the next matrices by size and optionaly closing the file
object* readMatrices(FILE* fp, int* size, int toClose)
{
    object* matrices;
    fscanf(fp, "%d\n", size);
    matrices = (object*)calloc((*size), sizeof(object));
    if (!matrices)
    {
        printf("problem in memory allocation!\n");
        return NULL;
    }
    
    for (int i = 0; i < *size; i++)
        matrices[i] = readMat(fp);
    if (toClose)
        fclose(fp);
    return matrices;
}

// function to read and allocate memory for the matrix
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
            fscanf(fp, "%d ", &mat.members[j][k]);
        fscanf(fp, "%d\n", &mat.members[j][mat.dim - 1]);
    }
    return mat;
}

// function to get a result if there is a matching between the obj and in picture in the current position
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
            if (diff > value) // if the difference higher than the matching value there is no need to continue calculation
                return 0;
        }
    }
    return 1;
}

// function of math absolute function but returning a double  
double myAbs(double num)
{
    if (num < 0)
        return num * -1;
    return num;
}

// finction that searching of the object in the all picture positions
int findSubMat(object pic, object obj, int matchVal, int* row, int* col)
{
    for (int i = 0; i < pic.dim; i++)
    {
        for (int j = 0; j < pic.dim; j++)
        {
            if (i + obj.dim < pic.dim && j + obj.dim < pic.dim)
            {
                if (matching(pic, i, j, obj, matchVal))
                {
                    *row = i;
                    *col = j;
                    return 1;
                }
            }
        }
    }
    return 0;   
}

// function to copy picture/object from one memory location to another in the same computer
void copyPic(object* to, object* from)
{
    to->id = from->id;
    to->dim = from->dim;
    to->members = (int**)calloc(to->dim, sizeof(int*));
    for (int row = 0; row < to->dim; row++)
    {
        to->members[row] = (int*)calloc(to->dim, sizeof(int));
        for (int col = 0; col < to->dim; col++)
            to->members[row][col] = from->members[row][col];
    }
}