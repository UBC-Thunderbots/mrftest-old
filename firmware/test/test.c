#include <stdlib.h>
#include <stdio.h>
#include "check.h"
#include "test.h"
#include "physics_test.c"
#include "quadratic_test.c"
#include "shoot_test.c"
#include "move_test.c"

static int number_failed = 0;

void run_test(TCase *tc, Suite *s) {
    suite_add_tcase(s, tc);
    SRunner *sr = srunner_create(s);
    srunner_run_all(sr, CK_NORMAL);
    number_failed += srunner_ntests_failed(sr);
    srunner_free(sr);
    printf("\n");
}

int main(void)
{
    printf("\nStart Tests\n");
    run_physics_test();
    run_quadratic_test();
    run_move_test();
    (number_failed == 0) ? printf("All tests passed.\n") : printf("%d Tests failed.\n\n", number_failed);
    return 0;
}