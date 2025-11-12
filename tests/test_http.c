#include "config.h"

#include "mock/mem.h"
#include "net/http.h"

static void
test_cippv6(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, CIPPV6_DOMAIN, 443, IPV6_ONLY) == 0) {
        const char *body = http_get(http, &gstr_from_const("/whatever"));
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
        const char *body = http_get(http, &gstr_from_const("/whatever"));
        WITH_SITE(body != NULL) { assert_int_equal(strlen(body), 4); }
        http_free(http);
    }
}

static void
test_post_with_cookie(void **state) {
    http_t *http = alloca(HTTP_T_SIZE);

    WITH_SITE(http_init(http, TEST_DIR "/assets/long_cookie.txt", 80,
                        (IPV4_IPV6 | HTTP_COOKIEJAR)) == 0) {
        const char *body = http_get(http, &gstr_from_const("/whatever"));
        WITH_SITE(body != NULL) {
            /* Post with cookie*/
            body = http_request(http, &gstr_from_const("/whatever"),
                                &gstr_from_const("a=1&1=1"));
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
    };

    DISCOVER();

    /* 正常运行模式：mock 在 constructor 中已用 TEST_FAIL_SITE 设置好 */
    return cmocka_run_group_tests(tests, NULL, NULL);
}
