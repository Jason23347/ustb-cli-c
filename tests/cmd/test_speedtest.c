#include "config.h"

#include "mock/mem.h"

#include "cmd/cmd.h"

#include <stdlib.h>

/* cmd_speedtest: 可调用且不崩溃，返回 EXIT_SUCCESS 或 EXIT_FAILURE */
void
test_speedtest(void **state) {
    char *argv[] = {"speedtest"};
    int r = cmd_speedtest(1, argv);
    assert_true(r == EXIT_SUCCESS || r == EXIT_FAILURE);
}
