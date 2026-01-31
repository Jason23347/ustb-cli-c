#include "config.h"

#include "cmd/test_account.c"
#include "cmd/test_balance.c"
#include "cmd/test_speedtest.c"
#include "mock/mem.h"

int
main(int argc, char **argv) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_fee),
        cmocka_unit_test(test_info),
        cmocka_unit_test(test_login_no_env_file),
        cmocka_unit_test(test_login_normal),
        cmocka_unit_test(test_logout),
        cmocka_unit_test(test_whoami_without_nid),
        cmocka_unit_test(test_whoami_all),
        cmocka_unit_test(test_devices),
        cmocka_unit_test(test_speedtest),
    };

    DISCOVER();

    /* 正常运行模式：mock 在 constructor 中已用 TEST_FAIL_SITE 设置好 */
    return cmocka_run_group_tests(tests, NULL, NULL);
}
