#include "physbot.h"
#include "quadratic.h"
#include "../physics.h"
#include "../dr.h"
#include "../cvxgen/solver.h"
#include <math.h>

// Angles of each wheel relative to the front of the bot.
// TODO: these should actually be measured.
const float rel_wheel_angles[4] = { 30, 120, -30, -120 };

/**
 * Builds the M matrix for the optimization. This matrix has one 
 * row for each acceleration (major, minor, rotational) and one 
 * column for each wheel on the bot.
 * 
 * @param pb a PhysBot that should be setup by the setup_bot function
 * in physbot.c
 * @param state The current state of the robot that should have the
 * @param M a 3 x 4 matrix where each column corresponds to a wheel
 * \
 */ 
void build_M_matrix(PhysBot pb, dr_data_t state, float M[3][4]) {
    int wheel_spin_direction;
    if (pb.rot.disp >= 0) {
        wheel_spin_direction = 1;
    } else {
        wheel_spin_direction = -1;
    }
    int i;
    for (i = 0; i < 4; i++) {
        float radians = rel_wheel_angles[i] * M_PI / 180.0f;
        float wheel_direction = state.angle + radians + (M_PI / 2.0f);
        float wheel_vector[2] = { cos(wheel_direction), sin(wheel_direction) };
        float major_row = dot2D(pb.major_vec, wheel_vector);
        float minor_row = dot2D(pb.minor_vec, wheel_vector);
        M[0][i] = major_row;
        M[1][i] = minor_row;
        M[2][i] = wheel_spin_direction;
    }
}

/**
 * Takes the Q matrix and converts it to a 16 length matrix for the 
 * CVXGEN solver.
 * 
 * @param Q A 4 x 4 matrix that is the result of multiplying M.T * M
 */
void to_1d_matrix(float Q[4][4]) {
    int i;
    int j;
    int k = 0;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            params.Q[k] = Q[i][j];
            k++;
        }
    }
}

/**
 * Creates the c matrix for the optimization.
 * 
 * @param a_req The requested accelerations supplied by the primitive.
 * @param M a 3 x 4 matrix where each column corresponds to a wheel
 * 
 * c = 2 * a_req.T * M
 */
void build_c_matrix(float a_req[3], float M[3][4]) {
    int i;
    int j;
    for (i = 0; i < 4; i++) {
        params.c[i] = 0;
    }
    // loop each M col
    for (j = 0; j < 4; j++) {
        // loop each M row
        for (i = 0; i < 3; i++) {
            params.c[j] += (double) (a_req[i] * M[i][j]);
        }
        params.c[j] = 2 * params.c[j];
    }
}

/**
 * Transposes the M matrix.
 * 
 * @param M a 3 x 4 matrix where each column corresponds to a wheel
 * @param M_T the transpose of the M matrix
 */ 
void transpose(float M[3][4], float M_T[4][3]) {
    int i; 
    int j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            M_T[j][i] = M[i][j];
        }
    }
}

/**
 * Multiplies the M.T * M to get the Q matrix for the optimization.
 * 
 * @param M a 3 x 4 matrix where each column corresponds to a wheel
 * @param M_T the transpose of the M matrix
 * @param Q A 4 x 4 matrix that is the result of multiplying M.T * M
 */ 
void build_Q_matrix(float M[3][4], float M_T[4][3], float Q[4][4]) {
    int i;
    int j;
    int k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            for (k = 0; k < 3; k++) {
                Q[i][j] = M_T[i][k] * M[k][j];
            }
        }
    }
}


/**
 * TODO: Figure out the units for the matrices so we make sure
 * that we get accelerations out of the optimization.
 */
void quad_optimize(PhysBot pb, dr_data_t state, float a_req[3]) {
    float M[3][4];
    float M_T[4][3];
    float Q[4][4];
    build_M_matrix(pb, state, M);
    transpose(M, M_T);
    build_c_matrix(a_req, M);
    build_Q_matrix(M, M_T, Q);
    set_defaults();
    setup_indexing();
    to_1d_matrix(Q);
    solve();
    double x = *vars.x;
    // TODO: remove this print statement once we figure out what 
    // parameters we need from the solver
    printf("%f\n", x);

}