#include "config.h"

#include "calc/fee.c"
#include "calc/flow.c"
#include "calc/timer.c"
#define main __main
#include "text.c"
#undef main

#define BUF_SIZE 16

static void
test_decimal_zero(void **state) {
    const int64_t number = 0;
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), number, 4);
    assert_string_equal(tmp, "0");
}

static void
test_decimal_normal(void **state) {
    const int64_t number = 114514;
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), number, 4);
    assert_string_equal(tmp, "11.45");
}

static void 
test_decimal_minus(void **state) {
    const int64_t number = -31415;

    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), number, 4);
    assert_string_equal(tmp, "-3.14");
}

int
main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_decimal_zero),
        cmocka_unit_test(test_decimal_normal),
        cmocka_unit_test(test_decimal_minus),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
