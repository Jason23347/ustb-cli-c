#include "config.h"

#include "lib/gbuff.c"

/* Test gbuff_init with valid size */
static void
test_gbuff_init_success(void **state) {
    gbuff_t buff = {0};
    int result = gbuff_init(&buff, 100);

    assert_int_equal(result, 0);
    assert_non_null(buff.data);
    assert_int_equal(buff.len, 0);
    assert_int_equal(buff.cap, 100);

    gbuff_free(&buff);
}

/* Test gbuff_init with zero size */
static void
test_gbuff_init_zero_size(void **state) {
    gbuff_t buff = {0};
    int result = gbuff_init(&buff, 0);

    assert_int_equal(result, 0);
    assert_non_null(buff.data);
    assert_int_equal(buff.cap, 0);

    gbuff_free(&buff);
}

/* Test gbuff_clear */
static void
test_gbuff_clear(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 50);
    gbuff_put(&buff, "hello", 5);

    assert_int_equal(buff.len, 5);

    gbuff_clear(&buff);
    assert_int_equal(buff.len, 0);
    assert_int_equal(buff.data[0], '\0');

    gbuff_free(&buff);
}

/* Test gbuff_put with sufficient space */
static void
test_gbuff_put_success(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 100);

    ssize_t result = gbuff_put(&buff, "hello", 5);

    assert_int_equal(result, 5);
    assert_int_equal(buff.len, 5);
    assert_memory_equal(buff.data, "hello", 5);

    gbuff_free(&buff);
}

/* Test gbuff_put with insufficient space */
static void
test_gbuff_put_insufficient_space(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 5);

    ssize_t result = gbuff_put(&buff, "hello", 5);

    assert_int_equal(result, -1);

    gbuff_free(&buff);
}

/* Test gbuff_put multiple times */
static void
test_gbuff_put_multiple(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 50);

    gbuff_put(&buff, "hello", 5);
    gbuff_put(&buff, " ", 1);
    gbuff_put(&buff, "world", 5);

    assert_int_equal(buff.len, 11);
    assert_memory_equal(buff.data, "hello world", 11);

    gbuff_free(&buff);
}

/* Test gbuff_ensure with sufficient capacity */
static void
test_gbuff_ensure_sufficient(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 100);

    int result = gbuff_ensure(&buff, 50);

    assert_int_equal(result, 0);
    assert_int_equal(buff.cap, 100);

    gbuff_free(&buff);
}

/* Test gbuff_ensure with insufficient capacity */
static void
test_gbuff_ensure_expand(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 50);

    int result = gbuff_ensure(&buff, 100);

    assert_int_equal(result, 0);
    assert_int_equal(buff.cap, 100);

    gbuff_free(&buff);
}

/* Test gbuff_realloc expand */
static void
test_gbuff_realloc_expand(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 50);
    gbuff_put(&buff, "hello", 5);

    int result = gbuff_realloc(&buff, 100);

    assert_int_equal(result, 0);
    assert_int_equal(buff.cap, 100);
    assert_int_equal(buff.len, 5);
    assert_memory_equal(buff.data, "hello", 5);

    gbuff_free(&buff);
}

/* Test gbuff_realloc shrink with truncation */
static void
test_gbuff_realloc_shrink(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 100);
    gbuff_put(&buff, "hello world", 11);

    int result = gbuff_realloc(&buff, 8);

    assert_int_equal(result, 0);
    assert_int_equal(buff.cap, 8);
    assert_int_equal(buff.len, 7);
    assert_int_equal(buff.data[7], '\0');

    gbuff_free(&buff);
}

/* Test gbuff_appendf basic formatting */
static void
test_gbuff_appendf_basic(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 50);

    int result = gbuff_appendf(&buff, "hello %s", "world");

    assert_true(result >= 0);
    assert_memory_equal(buff.data, "hello world", 11);

    gbuff_free(&buff);
}

/* Test gbuff_appendf with integers */
static void
test_gbuff_appendf_integers(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 50);

    int result = gbuff_appendf(&buff, "value: %d, hex: %x", 42, 255);

    assert_true(result >= 0);
    assert_memory_equal(buff.data, "value: 42, hex: ff", 18);

    gbuff_free(&buff);
}

/* Test gbuff_appendf overflow */
static void
test_gbuff_appendf_overflow(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 10);

    int result = gbuff_appendf(&buff, "this is a very long string");

    assert_true(result > 0);
    assert_int_equal(buff.len, 9);
    assert_int_equal(buff.data[9], '\0');

    gbuff_free(&buff);
}

/* Test gbuff_appendf on full buffer */
static void
test_gbuff_appendf_full_buffer(void **state) {
    gbuff_t buff = {0};
    gbuff_init(&buff, 10);
    gbuff_put(&buff, "123456789", 9);

    int result = gbuff_appendf(&buff, "x");

    assert_true(result < 0);

    gbuff_free(&buff);
}

/* Test gbuff_concat */
static void
test_gbuff_concat(void **state) {
    gbuff_t src = gbuff_from_const("world");
    gbuff_t dest = {0};
    gbuff_init(&dest, 50);
    gbuff_appendf(&dest, "hello ");

    int result = gbuff_concat(&dest, &src);

    assert_true(result >= 0);
    assert_memory_equal(dest.data, "hello world", 11);

    gbuff_free(&dest);
}

/* Test gbuff_extract with simple format */
static void
test_gbuff_extract_simple(void **state) {
    gbuff_t prefix = gbuff_from_const("key");
    gbuff_t fmt = gbuff_from_const("%d");
    int value = 0;

    struct extract data = {
        .prefix = &prefix,
        .fmt = &fmt,
        .src = "key=123",
        .dest = &value,
        .quoted = 0,
    };

    int result = gbuff_extract(&data);

    assert_int_equal(result, 1);
    assert_int_equal(value, 123);
}

/* Test gbuff_extract not found */
static void
test_gbuff_extract_not_found(void **state) {
    gbuff_t prefix = gbuff_from_const("key");
    gbuff_t fmt = gbuff_from_const("%d");
    int value = 0;

    struct extract data = {
        .prefix = &prefix,
        .fmt = &fmt,
        .src = "notfound=123",
        .dest = &value,
        .quoted = 0,
    };

    int result = gbuff_extract(&data);

    assert_int_equal(result, -1);
}

/* Test gbuff_extract with quoted values */
static void
test_gbuff_extract_quoted(void **state) {
    gbuff_t prefix = gbuff_from_const("name");
    gbuff_t fmt = gbuff_from_const("%5s");
    char value[10] = {0};

    struct extract data = {
        .prefix = &prefix,
        .fmt = &fmt,
        .src = "name='hello'",
        .dest = &value,
        .quoted = 1,
    };

    int result = gbuff_extract(&data);

    assert_true(result >= 1);
}

int
main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_gbuff_init_success),
        cmocka_unit_test(test_gbuff_init_zero_size),
        cmocka_unit_test(test_gbuff_clear),
        cmocka_unit_test(test_gbuff_put_success),
        cmocka_unit_test(test_gbuff_put_insufficient_space),
        cmocka_unit_test(test_gbuff_put_multiple),
        cmocka_unit_test(test_gbuff_ensure_sufficient),
        cmocka_unit_test(test_gbuff_ensure_expand),
        cmocka_unit_test(test_gbuff_realloc_expand),
        cmocka_unit_test(test_gbuff_realloc_shrink),
        cmocka_unit_test(test_gbuff_appendf_basic),
        cmocka_unit_test(test_gbuff_appendf_integers),
        cmocka_unit_test(test_gbuff_appendf_overflow),
        cmocka_unit_test(test_gbuff_appendf_full_buffer),
        cmocka_unit_test(test_gbuff_concat),
        cmocka_unit_test(test_gbuff_extract_simple),
        cmocka_unit_test(test_gbuff_extract_not_found),
        cmocka_unit_test(test_gbuff_extract_quoted),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
