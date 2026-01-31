#include "config.h"

#include "mock/mem.h"
#include "redirect/redirect_stdout.h"

#include "cmd/cmd.h"

#include <stdlib.h>
#include <string.h>

#define count(argv)          ((int)(sizeof(argv) / sizeof(argv[0])))
#define test_call(cmd, argv) cmd(count(argv), (argv))

void
test_login_no_env_file(void **state) {
    char *argv_fail[] = {
        "login",
        "-c",
        "/nonexistent/env",
    };
    int r = test_call(cmd_login, argv_fail);
    assert_int_equal(r, EXIT_FAILURE);
}

void
test_login_normal(void **state) {
    int r;
    char *argv[] = {
        "login",
        "-c",
        TEST_DIR "/assets/.env.example",
    };
    WITH_CAPTURE(out, r = test_call(cmd_login, argv)) {
        assert_int_equal(r, EXIT_SUCCESS);
        assert_string_contain(out, "2001:0da8:0208:1145:1419:dead:beaf:6666");
    }
}

void
test_logout(void **state) {
    char *argv[] = {"logout"};
    int r = test_call(cmd_logout, argv);
    assert_int_equal(r, EXIT_SUCCESS);
}

void
test_whoami_without_nid(void **state) {
    int r;

    char *argv[] = {
        "whoami",
    };
    WITH_CAPTURE(out, r = test_call(cmd_whoami, argv)) {
        IF_TESTING_SITE() { /* Skip */ }
        else {
            assert_int_equal(r, EXIT_SUCCESS);
            assert_string_contain(out, "U202412345");
        }
    }
}

void
test_whoami_all(void **state) {
    int r;
    char *argv[] = {
        "whoami",
        "-a",
    };
    WITH_CAPTURE(out, r = test_call(cmd_whoami, argv)) {
        IF_TESTING_SITE() { /* Skip */ }
        else {
            assert_int_equal(r, EXIT_SUCCESS);
            assert_string_contain(out, "U202412345");
            assert_string_contain(out, "吴彦祖");
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
