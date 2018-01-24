#include "test.h"
#include "check.h"
#include "../main/util/physbot.h"
#include "../main/primitives/move.h"

START_TEST(test_choose_rotation_destination)
{
    PhysBot pb = {
        .maj = {
            .disp = 2
        },
        .dr = {1, 0},
        .rot = {
            .disp = 0
        }
    };
    choose_rotation_destination(&pb, 0.0f);
    ck_assert_float_eq_tol((-CLOSEST_WHEEL_ANGLE + M_PI / 2.0f), pb.rot.disp, TOL);
}
END_TEST 


void run_move_test() {
    TCase *tc_core;
    Suite *s = suite_create("Move Test");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_choose_rotation_destination);
    run_test(tc_core, s);
}