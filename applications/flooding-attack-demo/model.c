#include <stdio.h>
#include <stdlib.h>

#include "model.h"
#include "./model_weights/rise-19.h" // CHANGE ME: model weights header

static Matrix buffer_layer1 = {1, 32, 0, 0};
static Matrix buffer_layer2 = {1, 32, 0, 0};

static float buffer_1[32] = {0};
static float buffer_2[NUM_INPUT_LENGTH] = {0};
static float output_buff[NUM_OUTPUT_LENGTH] = {0};

void Init_Detection()
{
    fixed_fc1_weight.elements = bin_fc1_weight;
    fixed_fc2_weight.elements = bin_fc2_weight;
    fixed_fc3_weight.elements = bin_fc3_weight;
    fixed_fc1_bias.elements = bin_fc1_bias;
    fixed_fc2_bias.elements = bin_fc2_bias;
    fixed_fc3_bias.elements = bin_fc3_bias;

    fixed_components.elements = bin_components;

    buffer_layer1.elements = buffer_1;
    buffer_layer2.elements = buffer_2;
}

void setRowsColumns(int16_t rows, int16_t columns, Matrix *matrix)
{
    matrix->rows = rows;
    matrix->columns = columns;
    return;
}

inline void setElement(int16_t row, int16_t column, float element, Matrix *matrix)
{
    matrix->elements[(row)*matrix->columns + (column)] = element;
    return;
}

inline float getElement(int16_t row, int16_t column, Matrix *matrix)
{
    return matrix->elements[(row)*matrix->columns + (column)];
}

void clearMatrix(Matrix *matrix)
{
    int num_elem = matrix->columns * matrix->rows;

    for (int i = 0; i < num_elem; i++)
    {
        matrix->elements[i] = 0;
    }

    return;
}

void Matmul(Matrix *matrixA, Matrix *matrixB, Matrix *matrixC, int verbose)
{
    /*  C = A @ B 
        C: (matrixA->rows, matrixB->columns) */

    if(verbose){

        printf("Shape of matrixA: (%d, %d)\n", matrixA->rows, matrixA->columns);
        printf("Content of matrixA: \n");
        for (int i = 0; i < matrixA->rows; i++)
        {
            for (int j = 0; j < matrixA->columns; j++)
            {
                printf("%f ", getElement(i, j, matrixA));
            }
            printf("\n\n");
        }

        printf("Shape of matrixB: (%d, %d)\n", matrixB->rows, matrixB->columns);
        printf("Content of matrixB: \n");
        for (int i = 0; i < matrixB->rows; i++)
        {
            for (int j = 0; j < matrixB->columns; j++)
            {
                printf("%f ", getElement(i, j, matrixB));
            }
            printf("\n\n");
        }
    }

    setRowsColumns(matrixA->rows, matrixB->columns, matrixC);

    // set the result matrix's rows and columns numbers and initialize its elements to zero
    clearMatrix(matrixC);

    // Implement matrix multiplication

    // for each row of matrixA
    for (int i = 0; i < matrixA->rows; i++)
    {
        // for each column of matrixB
        for (int j = 0; j < matrixB->columns; j++)
        {
            // for each element of matrixA's row
            for (int k = 0; k < matrixA->columns; k++)
            {
                // C[i][j] += A[i][k] * B[k][j]
                setElement(i, j, getElement(i, k, matrixA) * getElement(k, j, matrixB) + getElement(i, j, matrixC), matrixC);
            }
        }
    }

    if (verbose){
        printf("Shape of matrixC: (%d, %d)\n", matrixC->rows, matrixC->columns);
        printf("Content of matrixC: \n");
        for (int i = 0; i < matrixC->rows; i++)
        {
            for (int j = 0; j < matrixC->columns; j++)
            {
                printf("%f ", getElement(i, j, matrixC));
            }
            printf("\n\n");
        }
    }

}

void Relu(Matrix *matrix)
{
    int num_elem = matrix->columns * matrix->rows;
    for (int i = 0; i < num_elem; i++)
    {
        if (matrix->elements[i] < 0)
        {
            matrix->elements[i] = 0.0;
        }
    }

    return;
}

void bias_add(Matrix *matrix, Matrix *bias)
{
    for (int i = 0; i < bias->columns; i++)
    {
        // setElement(0, i, fixedpt_add(getElement(0, i, matrix), getElement(0, i, bias)), matrix);
        setElement(0, i, getElement(0, i, matrix) + getElement(0, i, bias), matrix);
    }
}

void standard_scale(Matrix *matrix)
{
    for (int i = 0; i < matrix->columns; i++)
    {
      setElement(0, i, (getElement(0, i, matrix) - bin_mean[i]) / bin_scale[i], matrix);
    }
}

int8_t RunInference_AttackDetection(Matrix *input, Matrix *output)
{
    Init_Detection();

    // input: {1, NUM_RAW_INPUT_LENGTH, 0, 0} == (NUM_RAW_INPUT_LENGTH, )
    // mean: (NUM_RAW_INPUT_LENGTH, )
    // scale: (NUM_RAW_INPUT_LENGTH, )
    // components: (NUM_RAW_INPUT_LENGTH, NUM_INPUT_LENGTH)

    // printf("Content of input (%d, %d): \n", input->rows, input->columns);
    // for (int i = 0; i < input->rows; i++)
    // {
    //     for (int j = 0; j < input->columns; j++)
    //     {
    //         printf("%f ", getElement(i, j, input));
    //     }
    //     printf("\n\n");
    // }
    standard_scale(input);


    Matmul(input, &fixed_components, &buffer_layer2, 0);


    Matmul(&buffer_layer2, &fixed_fc1_weight, &buffer_layer1, 0);
    bias_add(&buffer_layer1, &fixed_fc1_bias);
    Relu(&buffer_layer1);


    Matmul(&buffer_layer1, &fixed_fc2_weight, &buffer_layer2, 0);
    bias_add(&buffer_layer2, &fixed_fc2_bias);
    Relu(&buffer_layer2);


    Matmul(&buffer_layer2, &fixed_fc3_weight, output, 0);
    bias_add(output, &fixed_fc3_bias);
    Relu(output);

    int max_value = 0;
    int idx = 0;
    for (int i = 0; i < NUM_OUTPUT_LENGTH; i++)
    {

        if (output->elements[i] > max_value)
        {
            max_value = output->elements[i];
            idx = i;
        }
    }

    return idx;
}

int8_t AttackDectectionExample(float* input)
{
	Matrix input_vec = {1,NUM_RAW_INPUT_LENGTH, 0, 0};
	Matrix output_vec = {1, NUM_OUTPUT_LENGTH, 0, 0};
	
	input_vec.elements = (float*)input;
    output_vec.elements = output_buff;
    
    for (int i=0; i<NUM_OUTPUT_LENGTH; i++){
        output_vec.elements[i] = 0;
    }

	return RunInference_AttackDetection(&input_vec, &output_vec);
}
