#include "config.h"

#include "mock/mem.h"
#include "redirect/redirect_stdout.h"

#include "cmd/cmd.h"

#include <string.h>

void
test_fee(void **state) {
    WITH_CAPTURE(out, cmd_fee(0, NULL)) {
        IF_TESTING_SITE() { /* Skip */ }
        else {
            const char expected[] = "Money Cost: ￥0\n"
                                    "Money Left: ￥114.51\n\n";
            assert_string_equal(out, expected);
        }
    }
}

void
test_info(void **state) {
    WITH_CAPTURE(out, cmd_info(0, NULL)) {
        IF_TESTING_SITE() { /* Skip */ }
        else {
            /* 与 test_fee 一致：基于 mock 返回的 login_normal.txt 校验关键内容 */
            assert_non_null(out);
            assert_true(strstr(out, "114.51.41.19") != NULL);
            assert_true(strstr(out, "Flow used:\t45.11 GB") != NULL);
            assert_true(strstr(out, "Flow left:\t74.89 GB") != NULL);
            assert_true(strstr(out, "2001:0da8::6666") != NULL);
            assert_true(strstr(out, "Flow used:\t1.00 TB") != NULL);
            assert_true(strstr(out, "Flow saving rate (%): 0.95") != NULL);
        }
    }
}
