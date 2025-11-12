#include "config.h"

#include "cmd/test_balance.c"
#include "mock/mem.h"

int
main(int argc, char **argv) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fee),
    };

    DISCOVER();

    /* 正常运行模式：mock 在 constructor 中已用 TEST_FAIL_SITE 设置好 */
    return cmocka_run_group_tests(tests, NULL, NULL);
}
