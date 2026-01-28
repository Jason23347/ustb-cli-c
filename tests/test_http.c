#include "config.h"

#include "mock/mem.h"
#include "net/http.h"

static void
test_cippv6(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, CIPPV6_DOMAIN, 443, IPV6_ONLY) == 0) {
        const char *body = http_get(http, &gbuff_from_const("/whatever"));
        WITH_SITE(body != NULL) {
            const char raw_text[] =
                "gIpV6Addr = '2001:0da8:0208:1145:1419:dead:beaf:6666';";
            assert_memory_equal(body, raw_text, strlen(raw_text));
        }
        http_free(http);
    }
}

static void
test_login_normal(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, LOGIN_HOST, 80, IPV4_IPV6) == 0) {
        const char *body = http_get_root(http);
        WITH_SITE(body != NULL) { assert_int_equal(strlen(body), 2920); }
        http_free(http);
    }
}

static void
test_cookiejar(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, TEST_DIR "/assets/long_cookie.txt", 80,
                        (IPV4_IPV6 | HTTP_COOKIEJAR)) == 0) {
        const char *body = http_get(http, &gbuff_from_const("/whatever"));
        WITH_SITE(body != NULL) { assert_int_equal(strlen(body), 4); }
        http_free(http);
    }
}

static void
test_post_with_cookie(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, TEST_DIR "/assets/long_cookie.txt", 80,
                        (IPV4_IPV6 | HTTP_COOKIEJAR)) == 0) {
        const char *body = http_get(http, &gbuff_from_const("/whatever"));
        WITH_SITE(body != NULL) {
            /* Post with cookie*/
            body = http_request(http, &gbuff_from_const("/whatever"),
                                &gbuff_from_const("a=1&1=1"));
        }
        http_free(http);
    }
}

static void
test_status_code(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, LOGIN_HOST, 80, IPV4_IPV6) == 0) {
        const char *body = http_get_root(http);
        WITH_SITE(body != NULL) {
            int status = http_status_code(http);
            assert_int_equal(status, 200);
        }
        http_free(http);
    }
}

static void
test_disable_enable_redirect(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, LOGIN_HOST, 80,
                        (IPV4_IPV6 | HTTP_REDIRECT)) == 0) {
        /* 初始应该启用重定向 */
        const char *body = http_get_root(http);
        WITH_SITE(body != NULL) {
            /* 禁用重定向 */
            http_disable_redirect(http);
            /* 重新请求 */
            body = http_get_root(http);
            WITH_SITE(body != NULL) {
                /* 启用重定向 */
                http_enable_redirect(http);
                /* 再次请求 */
                body = http_get_root(http);
            }
        }
        http_free(http);
    }
}

static void
test_redirect(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    /* 测试重定向功能 - 使用完整的 URI 来测试 get_path_of_uri */
    /* Location 头包含完整 URI (https://example.com/target)，
     * get_path_of_uri 应该提取路径部分 (/target) */
    WITH_SITE(http_init(http, TEST_DIR "/assets/redirect_test.txt", 80,
                        (IPV4_IPV6 | HTTP_REDIRECT)) == 0) {
        /* 请求应该触发重定向逻辑 */
        const char *body = http_get(http, &gbuff_from_const("/source"));
        WITH_SITE(body != NULL) {
            /* 验证状态码是 302（重定向） */
            int status = http_status_code(http);
            assert_int_equal(status, 302);
        }
        http_free(http);
    }
}

int
main(int argc, char **argv) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_cippv6),
        cmocka_unit_test(test_login_normal),
        cmocka_unit_test(test_cookiejar),
        cmocka_unit_test(test_post_with_cookie),
        cmocka_unit_test(test_status_code),
        cmocka_unit_test(test_disable_enable_redirect),
        cmocka_unit_test(test_redirect),
    };

    DISCOVER();

    /* 正常运行模式：mock 在 constructor 中已用 TEST_FAIL_SITE 设置好 */
    return cmocka_run_group_tests(tests, NULL, NULL);
}
