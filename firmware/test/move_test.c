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

START_TEST(test_choose_rotation_destination_below_distance_limit)
{
    PhysBot pb = {
        .maj = {
            .disp = APPROACH_LIMIT / 2.0f
        },
        .dr = {1, 0},
        .rot = {
            .disp = -12345
        }
    };
    choose_rotation_destination(&pb, 0.0f);
    ck_assert_float_eq(-12345, pb.rot.disp);
}
END_TEST

START_TEST(test_plan_move_rotation_large) 
{
    PhysBot pb = {
        .maj = {
            .time = 0.0f
        },
        .rot = {
            .disp = 20 * M_PI / 180.0f
        }
    };
    plan_move_rotation(&pb, 3.0f);
    ck_assert_double_eq(TIME_HORIZON, pb.rot.time);
    ck_assert_double_eq_tol(27.925, pb.rot.vel, 0.001);
    ck_assert_double_eq_tol(MAX_T_A, pb.rot.accel, 0.001);
}
END_TEST

START_TEST(test_plan_move_rotation_small) 
{
    PhysBot pb = {
        .maj = {
            .time = 0.0f
        },
        .rot = {
            .disp = 2.0f * M_PI / 180.0f
        }
    };
    plan_move_rotation(&pb, 2.0f);
    ck_assert_double_eq(TIME_HORIZON, pb.rot.time);
    ck_assert_double_eq_tol(2.7925, pb.rot.vel, 0.001);
    ck_assert_double_eq_tol(15.851, pb.rot.accel, 0.001);
}
END_TEST

/**
 * Test function manager for move.c primitive
 */ 
void run_move_test() {
    // Put the name of the suite of tests in here
    Suite *s = suite_create("Move Test");
    // Creates a test case that you can add all of the tests to
    TCase *tc_core = tcase_create("Core");
    // add the tests for this file here
    tcase_add_test(tc_core, test_choose_rotation_destination);
    tcase_add_test(tc_core, test_choose_rotation_destination_below_distance_limit);
    tcase_add_test(tc_core, test_plan_move_rotation_large);
    tcase_add_test(tc_core, test_plan_move_rotation_small);
    // run the tests
    run_test(tc_core, s);
}