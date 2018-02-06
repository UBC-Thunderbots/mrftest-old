#include <stdlib.h>
#include <stdio.h>
#include "check.h"
#include "test.h"
#include "physics_test.c"
#include "quadratic_test.c"
#include "shoot_test.c"
#include "move_test.c"
<<<<<<< HEAD
#include "math_test.c"
=======
>>>>>>> 711f63a9dd52a459265a25a622a9dbd34c8d8f3a

static int number_failed = 0;

void run_test(TCase *tc, Suite *s) {
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    printf("\n");
}

/**
 * Main entry point for the test cases. Each test to run should 
 * be wrapped inside a function that should be added here so that
 * when the main function is called each test is run.
 */
int main(void)
{
    printf("\nStart Tests\n\n");
    run_physics_test();
    run_quadratic_test();
    run_move_test();
    run_math_test();
    (number_failed == 0) ? printf("All tests passed.\n") : printf("%d Tests failed.\n\n", number_failed);
    return 0;
}