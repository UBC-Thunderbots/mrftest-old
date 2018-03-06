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