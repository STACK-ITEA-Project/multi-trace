#ifndef _MLP_H_
#define _MLP_H_


// Constant(s)

#include <sys/types.h>
#include <inttypes.h>
#include <stdio.h>

#define FDATK_NCOL 17 // number of data columns per client

#define MAX_ELEMENTS (256*256)

// Type(s)

typedef struct MatrixStruct
{
    int32_t rows;
    int32_t columns;
    int transposed;
    float* elements;
} Matrix;

void setElement(int16_t row, int16_t column, float element, Matrix *matrix);
/**
 * Get element of matrix[row, col]
 */
float getElement(int16_t row, int16_t column, Matrix *matrix);
/**
 * 
 */
void setRowsColumns(int16_t rows, int16_t columns, Matrix *matrix);

/**
 * Run Inference Attact Detection.
 * Return values:
 * ATTACK_DETECTION_NO 0
 * ATTACK_DETECTION_DIO_DROP 1
 * ATTACK_DETECTION_DIS_REPEAT_MUL 2
 * ATTACK_DETECTION_DIS_REPEAT 3
 */
int8_t RunInference_AttackDetection(Matrix* input, Matrix* output);

void Matmul(Matrix *matrixA, Matrix *matrixB, Matrix *matrixC, int verbose);
void Relu(Matrix *matrix);
void bias_add(Matrix *matrix, Matrix *bias);

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
