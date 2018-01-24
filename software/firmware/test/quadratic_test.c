#include "quadratic_test.h"
#include "../main/util/quadratic.h"
#include "../main/util/physbot.h"
#include "../main/dr.h"
#include <stdbool.h>
#include <math.h>

bool quadratic_test() {
    dr_data_t states;
    PhysBot pb = {
        .rot = {
            .disp = 30.0f * M_PI / 180.0f
        },
        .major_vec = {0.0f, 1.0f},
        .minor_vec = {0.0f, 0.0f}
    };

    states.angle = 0;
    float a_req[3] = {1, 0, 0};
    float forces[4] = {};
    quad_optimize(forces, pb, states, a_req);
    return false;
}