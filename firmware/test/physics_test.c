#include "../main/physics.h"
#include "test.h"
#include "check.h"
 

START_TEST(test_dot2D)
{
    float a1[2] = {1.1f, 1.35f};
    float a2[2] = {2.6f, 5.1f};
    ck_assert_float_eq(9.745f, dot2D(a1, a2));
}
END_TEST

START_TEST(test_dot3D) 
{
    float a1[3] = {1, 1, 1};
    float a2[3] = {2, 2, 2};
    ck_assert_float_eq(6, dot3D(a1, a2));
}
END_TEST

START_TEST(test_dot_product)
{
    float a1[5] = {1, 1, 1, 1, 2};
    float a2[5] = {1, 2, 3, 4, 5};
    ck_assert_float_eq(20, dot_product(a1, a2, 5));
}
END_TEST

START_TEST(test_min_angle_delta_lt_90) 
{
    ck_assert_float_eq(30 * M_PI / 180.0f, min_angle_delta(0, 30 * M_PI / 180.0f));
}
END_TEST

START_TEST(test_min_angle_delta_gt_90) 
{
    ck_assert_float_eq(117 * M_PI / 180.0f, min_angle_delta(0, 117 * M_PI / 180.0f));
}
END_TEST

START_TEST(test_min_angle_delta_both_non_zero)
{
    ck_assert_float_eq_tol(-47 * M_PI / 180.0f, min_angle_delta(15 * M_PI / 180.0f, -32 * M_PI / 180.0f), TOL);
}
END_TEST

START_TEST(test_min_angle_delta_big_angles)
{
    ck_assert_float_eq_tol(-173 * M_PI / 180.0f, min_angle_delta(4 * M_PI, 187 * M_PI / 180.0f), TOL);
}
END_TEST

void run_physics_test() {
    TCase *tc_core;
    Suite *s = suite_create("Physics Test");

    /* Core test case */
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_dot2D);
    tcase_add_test(tc_core, test_dot3D);
    tcase_add_test(tc_core, test_dot_product);
    tcase_add_test(tc_core, test_min_angle_delta_lt_90);
    tcase_add_test(tc_core, test_min_angle_delta_gt_90);
    tcase_add_test(tc_core, test_min_angle_delta_both_non_zero);
    tcase_add_test(tc_core, test_min_angle_delta_big_angles);
    run_test(tc_core, s);
}