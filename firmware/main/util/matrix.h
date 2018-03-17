
// the variables that determine the size of the matrices used
// in matmul
static int n_rows, n_cols, m_rows, m_cols, out_rows, out_cols;

/**
 * Initialize all of values we need for matrix multiplication
 * 
 * @param _n_rows   rows in the left matrix
 * @param _n_cols   cols in the left matrix
 * @param _m_rows   rows in the right matrix
 * @param _m_cols   cols in the right matrix
 * @param _out_rows rows in the output matrix
 * @param _out_cols cols in the output matrix
 * @return void
 */
void set_vars(int _n_rows, int _n_cols, int _m_rows, int _m_cols, 
              int _out_rows, int _out_cols);

/**
 * Multiplies two matrices together. Must have called set_vars first to
 * set the sizes of the input and output matrices.
 * 
 * @param A the left matrix
 * @param B the right matrix
 * @param C the output matrix
 * @return void
 */
void matmul(float A[n_rows][n_cols], float B[m_rows][m_cols], 
            float C[out_rows][out_cols]);

/**
 * Rotates the coordinate axis through the angle represented by the given
 * unit vector such that the given vector is in a new reference frame.
 * The unit vector should be of the form {cos(theta), sin(theta)},
 * where theta is the angle you want to rotate the axis by.
 *
 * @param vector the vector to rotate
 * @param unit_vector {cos(theta), sin(theta)} of the angle you want to rotate the axis by
 */
void rotate_axis_2D(float vector[2], float unit_vector[2]);

/**
 * Rotates the given vector through the angle represented by the given
 * unit vector. The unit vector should be of the form {cos(theta), sin(theta)},
 * where theta is the angle you want to rotate the vector by.
 *
 * @param vector the vector to rotate
 * @param unit_vector {cos(theta), sin(theta)} of the angle you want to rotate the vector by
 */
void rotate_vector_2D(float vector[2], float unit_vector[2]);