#include "matrix_test.h"
#include "check.h"
#include "test.h"
#include "main/util/matrix.h"
#include <math.h>

START_TEST(test_matmul_vectors)
{
    float A[1][2] = {
        {5.0f, 3.0f}
    };
    float B[2][1] = {
        {4.0f},
        {6.0f}
    };
    float C[1][1];
    set_vars(1, 2, 2, 1, 1, 1);
    matmul(A, B, C);
    ck_assert_float_eq_tol(38.0f, C[0][0], TOL);
}
END_TEST

START_TEST(test_matmul_different_size)
{
    float A[2][3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f}
    };
    float B[3][2] = {
        {3.0f, 2.0f},
        {4.0f, 5.0f},
        {9.0f, 8.0f}
    };
    float expected_C[2][2] = {
        {38.0f, 36.0f},
        {86.0f, 81.0f}
    };
    float C[2][2];
    set_vars(2, 3, 3, 2, 2, 2);
    matmul(A, B, C);
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            ck_assert_float_eq_tol(expected_C[i][j], C[i][j], TOL);
        }
    }
}
END_TEST

START_TEST(test_matmul_same_size)
{
    float A[3][3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f}
    };
    float B[3][3] = {
        {3.0f, 2.0f, 1.0f},
        {4.0f, 5.0f, 6.0f},
        {9.0f, 8.0f, 7.0f}
    };
    float expected_C[3][3] = {
        {38.0f, 36.0f, 34.0f},
        {86.0f, 81.0f, 76.0f},
        {134.0f, 126.0f, 118.0f}
    };
    float C[3][3];
    set_vars(3, 3, 3, 3, 3, 3);
    matmul(A, B, C);
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            ck_assert_float_eq_tol(expected_C[i][j], C[i][j], TOL);
        }
    }
}
END_TEST

START_TEST(test_rotate_axis_2D)
{
    float vector[2] = {1.0f, 1.0f};
    float angle = (float) M_PI / 4.0f;
    float unit_vector[2] = {cosf(angle), sinf(angle)};
    rotate_axis_2D(vector, unit_vector);
    ck_assert_float_eq_tol(sqrtf(2.0f), vector[0], TOL);
    ck_assert_float_eq_tol(0.0f, vector[1], TOL);
}
END_TEST

START_TEST(test_rotate_vector_2D)
    {
        float vector[2] = {1.0f, 1.0f};
        float angle = (float) M_PI / 4.0f;
        float unit_vector[2] = {cosf(angle), sinf(angle)};
        rotate_vector_2D(vector, unit_vector);
        ck_assert_float_eq_tol(0.0f, vector[0], TOL);
        ck_assert_float_eq_tol(sqrtf(2.0f), vector[1], TOL);
    }
END_TEST

void run_matrix_test() {
    // Put the name of the suite of tests in here
    Suite *s = suite_create("Matrix Test");
    // Creates a test case that you can add all of the tests to
    TCase *tc = tcase_create("Core");
    // add the tests for this file here
    tcase_add_test(tc, test_matmul_vectors);
    tcase_add_test(tc, test_matmul_different_size);
    tcase_add_test(tc, test_matmul_same_size);
    tcase_add_test(tc, test_rotate_axis_2D);
    tcase_add_test(tc, test_rotate_vector_2D);
    // run the tests
    run_test(tc, s);
}