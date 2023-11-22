#ifndef _MLP_H_
#define _MLP_H_

// Constant(s)

#include <sys/types.h>
// #include "fixedpoint.h"
#include <inttypes.h>
#include <stdio.h>

#define MAX_ELEMENTS (256*256)


#define ATTACK_DETECTION_NO 0
#define ATTACK_DETECTION_DIO_DROP 1
#define ATTACK_DETECTION_DIS_REPEAT_MUL 2
#define ATTACK_DETECTION_DIS_REPEAT 3
#define NUM_INPUT_LENGTH 40

// Type(s)

typedef struct MatrixStruct
{
    int32_t rows;
    int32_t columns;
    int transposed;
    float* elements;
} Matrix;

// Function declarations

void Init_Detection();

/**
 * Read from file {path} and setup matrix elements.
 * matrix rows columns and memory allocation for elements are not initialized in this function. you need to do it manually.
 */
int SetFromFile(Matrix *matrix, const char* path);

/**
 * Clear the matrix elements into zero.
 */
void clearMatrix(Matrix *matrix);

/**
 * Convert floating point to fixed point array. 
 * Converison is performed in-place.
 */
void ConvertFloat2fixed(float* input, int elem_cnt);

/**
 *  Example function that run Attack detection model
 */
int8_t AttackDectectionExample(float* input);
#endif
