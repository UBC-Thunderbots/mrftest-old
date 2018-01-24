#include "physbot.h"
#include "quadratic.h"
#include "../physics.h"
#include "../dr.h"
#include "../cvxgen/solver.h"
#include <math.h>

const float rel_wheel_angles[4] = { 30, 120, -30, -120 };

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
        float wheel_direction = state.angle + radians + (wheel_spin_direction * M_PI / 2.0f);
        float wheel_vector[2] = { cos(wheel_direction), sin(wheel_direction) };
        float major_row = dot2D(pb.major_vec, wheel_vector);
        float minor_row = dot2D(pb.minor_vec, wheel_vector);
        M[0][i] = major_row;
        M[1][i] = minor_row;
        M[2][i] = wheel_spin_direction;
    }
}

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

void transpose(float M[3][4], float M_T[4][3]) {
    int i; 
    int j;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            M_T[j][i] = M[i][j];
        }
    }
}

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


void quad_optimize(float forces[4], PhysBot pb, dr_data_t state, float a_req[3]) {
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
    printf("%f", x);

}