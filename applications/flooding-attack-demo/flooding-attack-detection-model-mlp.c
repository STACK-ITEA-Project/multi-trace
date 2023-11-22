#include "flooding-attack-detection-model-mlp.h"

#include <stdio.h>


/**
 * Set element of matrix[row, col]
 */
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

void Matmul(Matrix *matrixA, Matrix *matrixB, Matrix *matrixC);
void Relu(Matrix *matrix);
void bias_add(Matrix *matrix, Matrix *bias);

/* Model Definition */ 
static Matrix fixed_fc1_weight = {32, NUM_INPUT_LENGTH, 1, 0};
static Matrix fixed_fc2_weight = {32, 32, 1, 0};
static Matrix fixed_fc3_weight = {4, 32, 1, 0};
static Matrix fixed_fc1_bias = {1, 32, 0, 0};
static Matrix fixed_fc2_bias = {1, 32, 0, 0};
static Matrix fixed_fc3_bias = {1, 4, 0, 0};
static Matrix buffer_layer1 = {1, 32, 0, 0};
static Matrix buffer_layer2 = {1, 32, 0, 0};

static float buffer_1[32];
static float buffer_2[32];
static float output_buff[4];

static float bin_fixed_fc1_weight[32 * NUM_INPUT_LENGTH] = { 0, };
static float bin_fixed_fc2_weight[32 * 32] = { 0, };
static float bin_fixed_fc3_weight[4 * 32] = { 0, };
static float bin_fixed_fc1_bias[1 * 32] = { 0, };
static float bin_fixed_fc2_bias[1 * 32] = { 0, };
static float bin_fixed_fc3_bias[1 * 4] = {0,  };


void Init_Detection()
{
    fixed_fc1_weight.elements = bin_fixed_fc1_weight;
    fixed_fc2_weight.elements = bin_fixed_fc2_weight;
    fixed_fc3_weight.elements = bin_fixed_fc3_weight;
    fixed_fc1_bias.elements = bin_fixed_fc1_bias;
    fixed_fc2_bias.elements = bin_fixed_fc2_bias;
    fixed_fc3_bias.elements = bin_fixed_fc3_bias;
    buffer_layer1.elements = buffer_1;
    buffer_layer1.elements = buffer_2;
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

void Matmul(Matrix *matrixA, Matrix *matrixB, Matrix *matrixC)
{
    setRowsColumns(matrixA->rows, matrixB->rows, matrixC);

    // set the result matrix's rows and columns numbers and initialize its elements to zero
    clearMatrix(matrixC);

    for (int B = 0; B < matrixB->rows; B++)
    {
        for (int A = 0; A < matrixA->columns; A++)
        {
            setElement(0, B, getElement(0, B, matrixC) + getElement(0, A, matrixA) * getElement(B, A, matrixB), matrixC);
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
    for (int i = 0; i < matrix->columns; i++)
    {
        // setElement(0, i, fixedpt_add(getElement(0, i, matrix), getElement(0, i, bias)), matrix);
        setElement(0, i, getElement(0, i, matrix) + getElement(0, i, bias), matrix);
    }
}

int8_t RunInference_AttackDetection(Matrix *input, Matrix *output)
{
    // Init_Detection();

    Matmul(input, &fixed_fc1_weight, &buffer_layer1);
    bias_add(&buffer_layer1, &fixed_fc1_bias);
    Relu(&buffer_layer1);

    Matmul(&buffer_layer1, &fixed_fc2_weight, &buffer_layer2);
    bias_add(&buffer_layer2, &fixed_fc2_bias);
    Relu(&buffer_layer2);

    Matmul(&buffer_layer1, &fixed_fc3_weight, output);
    bias_add(output, &fixed_fc3_bias);
    Relu(output);

    int max_value = 0;
    int idx = 0;
    for (int i = 0; i < 4; i++)
    {

        if (output->elements[i] * 100 > max_value)
        {
            max_value = output->elements[i];
            idx = i;
        }
    }

    return idx;
}

int8_t AttackDectectionExample(float* input)
{
	Matrix input_vec = {1, NUM_INPUT_LENGTH, 0, 0};
	Matrix output_vec = {1, 4, 0, 0};
	
	input_vec.elements = (float*)input;
    output_vec.elements = output_buff;
    output_buff[0] = 0.0;
    output_buff[1] = 0.0;
    output_buff[2] = 0.0;
    output_buff[3] = 0.0;

	// ConvertFloat2fixed(input, 1 * NUM_INPUT_LENGTH);
	return RunInference_AttackDetection(&input_vec, &output_vec);
}
