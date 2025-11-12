#ifndef MOCK_MEM_H
#define MOCK_MEM_H

#define IF_TESTING_SITE() if (test_get_fail_site() > 0)
#define WITH_SITE(expr)                                                        \
    if (!(expr)) {                                                             \
        IF_TESTING_SITE() {}                                                   \
        else {                                                                 \
            assert_true(!(expr));                                              \
        }                                                                      \
    } else

#define DISCOVER()                                                             \
    do {                                                                       \
        int discover = 0;                                                      \
        for (int i = 1; i < argc; ++i) {                                       \
            if (strcmp(argv[i], "--discover") == 0)                            \
                discover = 1;                                                  \
        }                                                                      \
        if (discover) {                                                        \
            test_reset_fail_site();                                            \
            cmocka_run_group_tests(tests, NULL, NULL);                         \
            mock_print_max_site_id();                                          \
            return 0;                                                          \
        }                                                                      \
    } while (0)

#define SHOULD_FAIL(mock, real)                                                \
    ({                                                                         \
        void *caller = __builtin_return_address(0);                            \
        should_fail_for_caller(caller) ? (mock) : (real);                      \
    })

#define WRAP(func) __wrap_##func
#define REAL(func) __real_##func

/* 返回当前全局设置的 fail site id (-1 表示未设置) */
int test_get_fail_site(void);
/* 测试接口（测试代码可以调用这些函数来控制故障注入） */
void test_set_fail_site(int site_id);
void test_set_fail_site_by_addr(void *addr);
void test_reset_fail_site(void);
int test_get_max_site_id(void);
int test_addr_to_site_id(void *addr); /* 返回 0 表示未知 */

void mock_print_max_site_id(void);
int should_fail_for_caller(void *caller_retaddr);

#endif /* MOCK_MEM_H */
