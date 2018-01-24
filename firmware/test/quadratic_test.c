#include "test.h"
#include "check.h"
#include "../main/util/quadratic.h"
#include <math.h>

START_TEST(quadratic_test) 
{
    PhysBot pb = {
        .rot = {
            .disp = 30.0f * M_PI / 180.0f
        },
        .major_vec = {1, 1},
        .minor_vec = {1, 0}
    };
    float a_req[3] = {1, 0, 0};
    dr_data_t state;
    state.angle = 0;
    quad_optimize(pb, state, a_req);
}
END_TEST

void run_quadratic_test() {
    TCase *tc_core;
    Suite *s = suite_create("Qudratic Test");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, quadratic_test);
    suite_add_tcase(s, tc_core);
    run_test(tc_core, s);
}