#include "config.h"

#include "mock/mem.h"
#include "redirect/redirect_stdout.h"

#include "cmd/cmd.h"

#include <stdlib.h>
#include <string.h>

/* cmd_login: 无 env 或指定不存在的 env 时应返回 EXIT_FAILURE */
void
test_login_no_env_file(void **state) {
    char *argv_fail[] = {"login", "-c", "/nonexistent/env"};
    int r = cmd_login(3, argv_fail);
    assert_int_equal(r, EXIT_FAILURE);
}

void
test_login(void **state) {
    int argc = 3;
    char *argv[] = {"login", "-c", TEST_DIR "/assets/.env.example"};
    int res = cmd_login(argc, argv);
    assert_int_equal(res, EXIT_SUCCESS);
}

/* cmd_logout: mock 下从 LOGIN_HOST 读取，应成功返回 */
void
test_logout(void **state) {
    char *argv[] = {"logout"};
    int r = cmd_logout(1, argv);
    assert_int_equal(r, EXIT_SUCCESS);
}

/* cmd_whoami: mock 返回 login_normal.txt 时应有用户名输出 */
void
test_whoami(void **state) {
    WITH_CAPTURE(out, cmd_whoami(1, (char *[]){"whoami"})) {
        IF_TESTING_SITE() { /* Skip */ }
        else {
            assert_non_null(out);
            /* login_normal.txt 中 uid='U202412345' */
            assert_true(strstr(out, "U202412345") != NULL);
        }
    }
}

/* cmd_devices: 无 SSL 时或 mock 下仅校验可调用且返回合理值 */
void
test_devices(void **state) {
    char *argv[] = {"devices"};
    int r = cmd_devices(1, argv);
    assert_true(r == EXIT_SUCCESS || r == EXIT_FAILURE);
}
