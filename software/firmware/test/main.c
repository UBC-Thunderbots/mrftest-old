#include <stdio.h>
#include "quadratic_test.h"
#include "main.h"
#include <stdbool.h>

void print_failure() {
    char fail[4] = { 'f', 'a', 'i', 'l'};
    printf("%s\n", fail);
}

int main() {
    if (quadratic_test() == false) {
        print_failure();
    }
    return 0;
}