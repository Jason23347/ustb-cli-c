#include "config.h"

#include "mock/mem.h"
#include "redirect/redirect_stdout.h"

#include "cmd.h"

#include <string.h>

void
test_fee(void **state) {
    WITH_CAPTURE(out, cmd_fee(0, NULL)) {
        IF_TESTING_SITE() { /* Skip */ }
        else {
            const char expected[] = "Money Cost: ￥0\n"
                                    "Money Left: ￥114.51\n";
            assert_memory_equal(out, expected, strlen(expected));
        }
    }
}
