#include "matrix.h"

void set_vars(int _n_rows, int _n_cols, int _m_rows, int _m_cols, 
              int _out_rows, int _out_cols) {
    n_rows = _n_rows;
    n_cols = _n_cols;
    m_rows = _m_rows;
    m_cols = _m_cols;
    out_rows = _out_rows;
    out_cols = _out_cols;
}


void matmul(float A[n_rows][n_cols], float B[m_rows][m_cols], 
            float C[out_rows][out_cols]) {
    int i;
    int j;
    int k;
    for (i = 0; i < m_cols; i++) {
        for (j = 0; j < n_rows; j++) {
            C[j][i] = 0.0f;
            for (k = 0; k < n_cols; k++) {
                C[j][i] += A[j][k] * B[k][i];
            }
        }
    }
}

/**
 * Rotates the given vector by using the given rotation matrix.
 *
 * @param vector a 2D vector to rotate
 * @param rotation_matrix the rotation operator
 */
void do_rotation(float vector[2], float rotation_matrix[2][2]) {
    float transposed_vector[2][1] = {
            {vector[0]},
            {vector[1]}
    };
    set_vars(2, 2, 2, 1, 2, 1);
    float vector_in_rotated_axis[2][1];
    matmul(rotation_matrix, transposed_vector, vector_in_rotated_axis);
    vector[0] = vector_in_rotated_axis[0][0];
    vector[1] = vector_in_rotated_axis[1][0];
}

void rotate_axis_2D(float vector[2], float unit_vector[2]) {
    float rotation_matrix[2][2] = {
            {unit_vector[0], unit_vector[1]},
            {-unit_vector[1], unit_vector[0]}
    };
    do_rotation(vector, rotation_matrix);
}

void rotate_vector_2D(float vector[2], float unit_vector[2]) {
    float rotation_matrix[2][2] = {
            {unit_vector[0], -unit_vector[1]},
            {unit_vector[1], unit_vector[0]}
    };
    do_rotation(vector, rotation_matrix);
}


void transpose(float in_matrix[n_rows][n_cols], float out_matrix[out_rows][out_cols]) {
    int i; 
    int j;
    for (i = 0; i < n_rows; i++) {
        for (j = 0; j < n_cols; j++) {
            out_matrix[j][i] = in_matrix[i][j];
        }
    }
}
