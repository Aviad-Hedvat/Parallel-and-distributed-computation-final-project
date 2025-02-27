#pragma once

#define INPUT_NAME "input.txt"
#define OUTPUT_NAME "output.txt"

// struct for using with pictures & objects
typedef struct
{
    int id;
    int dim;
    int** members;
} object;

FILE* readFile(double* value);
object* readMatrices(FILE* fp, int* size, int toClose);
object readMat(FILE* fp);
int matching(object picture, int row, int col, object obj, double value);
double myAbs(double num);
int findSubMat(object pic, object obj, int matchVal, int* row, int* col);
void copyPic(object* to, object* from);