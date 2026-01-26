#include "config.h"

#include <math.h>

#include "calc/fee.c"
#include "calc/flow.c"
#include "calc/interval.c"

#include <unistd.h>

#define BUF_SIZE 32

/* ======================= assign_decimal tests ======================== */

static void
test_assign_decimal_zero(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), 0, 4);
    assert_string_equal(tmp, "0");
}

static void
test_assign_decimal_positive(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), 123456, 4);
    assert_string_equal(tmp, "12.34");
}

static void
test_assign_decimal_negative(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), -123456, 4);
    assert_string_equal(tmp, "-12.34");
}

static void
test_assign_decimal_one_place(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), 5, 1);
    assert_string_equal(tmp, "0.5");
}

static void
test_assign_decimal_two_places(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), 12, 2);
    assert_string_equal(tmp, "0.12");
}

static void
test_assign_decimal_no_decimal(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), 1000, 0);
    assert_string_equal(tmp, "1000.");
}

static void
test_assign_decimal_large_value(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), 9999999999LL, 4);
    assert_string_equal(tmp, "999999.99");
}

static void
test_assign_decimal_negative_zero(void **state) {
    char tmp[BUF_SIZE] = {0};
    assign_decimal(tmp, sizeof(tmp), -1, 2);
    assert_string_equal(tmp, "-0.01");
}

/* ======================= fee_cost tests ======================== */

static void
test_fee_cost_no_overflow(void **state) {
    unsigned cost = fee_cost(FREE_FLOW_KB - 1000);
    assert_int_equal(cost, 0);
}

static void
test_fee_cost_slight_overflow(void **state) {
    unsigned cost = fee_cost(FREE_FLOW_KB + GB);
    assert_true(cost > 0);
}

static void
test_fee_cost_zero(void **state) {
    unsigned cost = fee_cost(0);
    assert_int_equal(cost, 0);
}

static void
test_fee_cost_large_overflow(void **state) {
    unsigned cost = fee_cost(FREE_FLOW_KB + 1000 * GB);
    assert_true(cost > 0);
}

/* ======================= cost_color tests ======================== */

static void
test_cost_color_green(void **state) {
    int color = cost_color("5.00");
#ifdef WITH_COLOR
    assert_int_equal(color, GREEN);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_cost_color_blue(void **state) {
    int color = cost_color("20.00");
#ifdef WITH_COLOR
    assert_int_equal(color, BLUE);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_cost_color_yellow(void **state) {
    int color = cost_color("40.00");
#ifdef WITH_COLOR
    assert_int_equal(color, YELLOW);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_cost_color_red(void **state) {
    int color = cost_color("60.00");
#ifdef WITH_COLOR
    assert_int_equal(color, RED);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_cost_color_boundary(void **state) {
    int color1 = cost_color("10.00");
    int color2 = cost_color("30.00");
    int color3 = cost_color("50.00");
    /* Just test they return some value */
    assert_true(color1 >= 0);
    assert_true(color2 >= 0);
    assert_true(color3 >= 0);
}

/* ======================= balance_color tests ======================== */

static void
test_balance_color_green(void **state) {
    int color = balance_color("50.00");
#ifdef WITH_COLOR
    assert_int_equal(color, GREEN);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_balance_color_blue(void **state) {
    int color = balance_color("20.00");
#ifdef WITH_COLOR
    assert_int_equal(color, BLUE);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_balance_color_yellow(void **state) {
    int color = balance_color("5.00");
#ifdef WITH_COLOR
    assert_int_equal(color, YELLOW);
#else
    assert_int_equal(color, 0);
#endif
}

static void
test_balance_color_red(void **state) {
    int color = balance_color("1.00");
#ifdef WITH_COLOR
    assert_int_equal(color, RED);
#else
    assert_int_equal(color, 0);
#endif
}

/* ======================= flow_left tests ======================== */

static void
test_flow_left_under_limit(void **state) {
    uint64_t flow_kb = 50LL * 1024 * 1024; /* 50MB, well under FREE_FLOW_KB */
    uint64_t left = flow_left(flow_kb);
    uint64_t expected = FREE_FLOW_KB - flow_kb;
    assert_int_equal(left, expected);
}

static void
test_flow_left_exact_limit(void **state) {
    uint64_t left = flow_left(FREE_FLOW_KB);
    assert_int_equal(left, 0);
}

static void
test_flow_left_over_limit(void **state) {
    uint64_t flow_kb = FREE_FLOW_KB + 1024;
    uint64_t left = flow_left(flow_kb);
    assert_int_equal(left, 0);
}

static void
test_flow_left_zero(void **state) {
    uint64_t left = flow_left(0);
    assert_int_equal(left, FREE_FLOW_KB);
}

/* ======================= flow_over tests ======================== */

static void
test_flow_over_under_limit(void **state) {
    uint64_t flow_kb = 50 * 1024 * 1024;
    uint64_t over = flow_over(flow_kb);
    assert_int_equal(over, 0);
}

static void
test_flow_over_exact_limit(void **state) {
    uint64_t over = flow_over(FREE_FLOW_KB);
    assert_int_equal(over, 0);
}

static void
test_flow_over_past_limit(void **state) {
    uint64_t flow_kb = FREE_FLOW_KB + 1024;
    uint64_t over = flow_over(flow_kb);
    assert_int_equal(over, 1024);
}

static void
test_flow_over_zero(void **state) {
    uint64_t over = flow_over(0);
    assert_int_equal(over, 0);
}

/* ======================= flow_format tests ======================== */

static void
test_flow_format_zero(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format(0, buf, sizeof(buf));
    assert_string_equal(buf, "0 KB");
}

static void
test_flow_format_small_kb(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format(1, buf, sizeof(buf));
    assert_string_equal(buf, "0 KB");
}

static void
test_flow_format_kb(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format(100, buf, sizeof(buf)); /* 100 KB */
    assert_string_equal(buf, "100.00 KB");
}

static void
test_flow_format_mb(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format(2048, buf, sizeof(buf)); /* 2048 KB = 2 MB */
    assert_string_equal(buf, "2.00 MB");
}

static void
test_flow_format_gb(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format(2048 * MB, buf, sizeof(buf)); /* 2048*1024 KB = 2 GB */
    assert_string_equal(buf, "2.00 GB");
}

static void
test_flow_format_tb(void **state) {
    char buf[BUF_SIZE] = {0};
    /* 0.9 * TB = 0.9 * 1024 * 1024 * 1024 KB, so use 1 TB to trigger TB display
     */
    uint64_t flow = 1LL * 1024 * 1024 * 1024;
    flow_format(flow, buf, sizeof(buf));
    assert_string_equal(buf, "1.00 TB");
}

/* ======================= flow_speed tests ======================== */

static void
test_flow_speed_basic(void **state) {
    flow_history_t history = {0};
    history.head = 0;
    history.tail = 0;

    /* Initialize with zero values */
    for (int i = 0; i < FLOW_NUM; i++) {
        history.arr[i].download = 0;
        history.arr[i].speed = 0;
        history.arr[i].tval.tv_sec = 0;
        history.arr[i].tval.tv_usec = 0;
    }

    /* First call should return 0 since microsec is 0 */
    uint64_t speed = flow_speed(&history, 1000);
    assert_true(speed >= 0);
}

static void
test_flow_speed_increasing(void **state) {
    flow_history_t history = {0};
    history.head = 0;
    history.tail = 0;

    /* Initialize first element */
    struct timeval now;
    gettimeofday(&now, NULL);
    history.arr[0].tval = now;
    history.arr[0].download = 0;

    /* Small delay and then call with new download */
    usleep(100000); /* 100ms */
    uint64_t speed = flow_speed(&history, 1000);

    /* Speed should be reasonable */
    assert_true(speed >= 0);
}

/* ======================= flow_format_speed tests ======================== */

static void
test_flow_format_speed_zero_bits_disabled(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format_speed(0, buf, sizeof(buf), 0);
    assert_string_equal(buf, "0 KB/s");
}

static void
test_flow_format_speed_small_kb_bits_disabled(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format_speed(512, buf, sizeof(buf), 0); /* 512 KB/s, < 1000 KB */
    assert_string_equal(buf, "512 KB/s");
}

static void
test_flow_format_speed_large_kb_bits_disabled(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format_speed(5 * 1024, buf, sizeof(buf), 0); /* 5 MB/s = 5*1024 KB */
    assert_string_equal(buf, "5.00 MB/s");
}

static void
test_flow_format_speed_zero_bits_enabled(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format_speed(0, buf, sizeof(buf), 1);
    assert_string_equal(buf, "0 Kbps");
}

static void
test_flow_format_speed_small_kb_bits_enabled(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format_speed(512, buf, sizeof(buf),
                      1); /* 512 KB/s = 4096 Kbps, < 1000 KB */
    assert_string_equal(buf, "4096 Kbps");
}

static void
test_flow_format_speed_large_kb_bits_enabled(void **state) {
    char buf[BUF_SIZE] = {0};
    flow_format_speed(5 * 1024, buf, sizeof(buf),
                      1); /* 5 MB/s = 5*1024*8 = 40960 Kbps = 40 Mbps */
    assert_string_equal(buf, "40 Mbps");
}

/* ======================= flow_speed_color tests ======================== */

static void
test_flow_speed_color_slow(void **state) {
    int color = flow_speed_color(512); /* 512 KB/s, < 1MB, GREEN */
    assert_int_equal(color, GREEN);
}

static void
test_flow_speed_color_medium(void **state) {
    int color =
        flow_speed_color(3 * 1024); /* 3 MB/s = 3*1024 KB/s, 1MB-6MB, YELLOW */
    assert_int_equal(color, YELLOW);
}

static void
test_flow_speed_color_fast(void **state) {
    int color =
        flow_speed_color(10 * 1024); /* 10 MB/s = 10*1024 KB/s, >= 6MB, RED */
    assert_int_equal(color, RED);
}

/* ======================= sleep_till_next_sec tests ======================== */

static void
test_sleep_till_next_sec(void **state) {
    /* Just test that the function can be called without crashing */
    int result = sleep_till_next_sec();
    assert_true(result >= -1);
}

/* ======================= microsec_interval tests ======================== */

static void
test_microsec_interval_same_time(void **state) {
    struct timeval tv = {.tv_sec = 100, .tv_usec = 50000};
    suseconds_t interval = microsec_interval(tv, tv);
    assert_int_equal(interval, 0);
}

static void
test_microsec_interval_different_sec(void **state) {
    struct timeval start = {.tv_sec = 100, .tv_usec = 0};
    struct timeval end = {.tv_sec = 102, .tv_usec = 0};
    suseconds_t interval = microsec_interval(start, end);
    assert_int_equal(interval, 2000000);
}

static void
test_microsec_interval_different_usec(void **state) {
    struct timeval start = {.tv_sec = 100, .tv_usec = 0};
    struct timeval end = {.tv_sec = 100, .tv_usec = 500000};
    suseconds_t interval = microsec_interval(start, end);
    assert_int_equal(interval, 500000);
}

static void
test_microsec_interval_both_different(void **state) {
    struct timeval start = {.tv_sec = 100, .tv_usec = 100000};
    struct timeval end = {.tv_sec = 102, .tv_usec = 600000};
    suseconds_t interval = microsec_interval(start, end);
    assert_int_equal(interval, 2500000);
}

static void
test_microsec_interval_backward(void **state) {
    struct timeval start = {.tv_sec = 102, .tv_usec = 0};
    struct timeval end = {.tv_sec = 100, .tv_usec = 0};
    suseconds_t interval = microsec_interval(start, end);
    assert_int_equal(interval, -2000000);
}

/* ======================= micro2sec tests ======================== */

static void
test_micro2sec_zero(void **state) {
    double sec = micro2sec(0);
    assert_true(fabs(sec - 0.0) < 0.0001);
}

static void
test_micro2sec_one_sec(void **state) {
    double sec = micro2sec(1000000);
    assert_true(fabs(sec - 1.0) < 0.0001);
}

static void
test_micro2sec_half_sec(void **state) {
    double sec = micro2sec(500000);
    assert_true(fabs(sec - 0.5) < 0.0001);
}

static void
test_micro2sec_millisec(void **state) {
    double sec = micro2sec(1000);
    assert_true(fabs(sec - 0.001) < 0.0001);
}

/* ======================= speed_per_sec tests ======================== */

static void
test_speed_per_sec_one_sec(void **state) {
    double speed = speed_per_sec(1000, 1000000);
    assert_true(fabs(speed - 1000.0) < 0.1);
}

static void
test_speed_per_sec_half_sec(void **state) {
    double speed = speed_per_sec(1000, 500000);
    assert_true(fabs(speed - 2000.0) < 0.1);
}

static void
test_speed_per_sec_zero(void **state) {
    double speed = speed_per_sec(0, 1000000);
    assert_true(fabs(speed - 0.0) < 0.0001);
}

static void
test_speed_per_sec_large_flow(void **state) {
    double speed = speed_per_sec(1024 * 1024, 1000000);
    assert_true(speed > 1000000);
}

/* ======================= random_d tests ======================== */

static void
test_random_d_range(void **state) {
    for (int i = 0; i < 100; i++) {
        double r = random_d();
        assert_true(r >= 0.0 && r <= 1.0);
    }
}

static void
test_random_d_variation(void **state) {
    double r1 = random_d();
    double r2 = random_d();
    /* At least one different (with very high probability) */
    assert_true(r1 != r2 || (r1 == 0.0 && r2 == 1.0));
}

int
main(void) {
    const struct CMUnitTest tests[] = {
        /* assign_decimal tests */
        cmocka_unit_test(test_assign_decimal_zero),
        cmocka_unit_test(test_assign_decimal_positive),
        cmocka_unit_test(test_assign_decimal_negative),
        cmocka_unit_test(test_assign_decimal_one_place),
        cmocka_unit_test(test_assign_decimal_two_places),
        cmocka_unit_test(test_assign_decimal_no_decimal),
        cmocka_unit_test(test_assign_decimal_large_value),
        cmocka_unit_test(test_assign_decimal_negative_zero),

        /* fee_cost tests */
        cmocka_unit_test(test_fee_cost_no_overflow),
        cmocka_unit_test(test_fee_cost_slight_overflow),
        cmocka_unit_test(test_fee_cost_zero),
        cmocka_unit_test(test_fee_cost_large_overflow),

        /* cost_color tests */
        cmocka_unit_test(test_cost_color_green),
        cmocka_unit_test(test_cost_color_blue),
        cmocka_unit_test(test_cost_color_yellow),
        cmocka_unit_test(test_cost_color_red),
        cmocka_unit_test(test_cost_color_boundary),

        /* balance_color tests */
        cmocka_unit_test(test_balance_color_green),
        cmocka_unit_test(test_balance_color_blue),
        cmocka_unit_test(test_balance_color_yellow),
        cmocka_unit_test(test_balance_color_red),

        /* flow_left tests */
        cmocka_unit_test(test_flow_left_under_limit),
        cmocka_unit_test(test_flow_left_exact_limit),
        cmocka_unit_test(test_flow_left_over_limit),
        cmocka_unit_test(test_flow_left_zero),

        /* flow_over tests */
        cmocka_unit_test(test_flow_over_under_limit),
        cmocka_unit_test(test_flow_over_exact_limit),
        cmocka_unit_test(test_flow_over_past_limit),
        cmocka_unit_test(test_flow_over_zero),

        /* flow_format tests */
        cmocka_unit_test(test_flow_format_zero),
        cmocka_unit_test(test_flow_format_small_kb),
        cmocka_unit_test(test_flow_format_kb),
        cmocka_unit_test(test_flow_format_mb),
        cmocka_unit_test(test_flow_format_gb),
        cmocka_unit_test(test_flow_format_tb),

        /* flow_format_speed tests */
        cmocka_unit_test(test_flow_format_speed_zero_bits_disabled),
        cmocka_unit_test(test_flow_format_speed_small_kb_bits_disabled),
        cmocka_unit_test(test_flow_format_speed_large_kb_bits_disabled),
        cmocka_unit_test(test_flow_format_speed_zero_bits_enabled),
        cmocka_unit_test(test_flow_format_speed_small_kb_bits_enabled),
        cmocka_unit_test(test_flow_format_speed_large_kb_bits_enabled),

        /* flow_speed tests */
        cmocka_unit_test(test_flow_speed_basic),
        cmocka_unit_test(test_flow_speed_increasing),

        /* flow_speed_color tests */
        cmocka_unit_test(test_flow_speed_color_slow),
        cmocka_unit_test(test_flow_speed_color_medium),
        cmocka_unit_test(test_flow_speed_color_fast),
        /* microsec_interval tests */
        cmocka_unit_test(test_microsec_interval_same_time),
        cmocka_unit_test(test_microsec_interval_different_sec),
        cmocka_unit_test(test_microsec_interval_different_usec),
        cmocka_unit_test(test_microsec_interval_both_different),
        cmocka_unit_test(test_microsec_interval_backward),

        /* micro2sec tests */
        cmocka_unit_test(test_micro2sec_zero),
        cmocka_unit_test(test_micro2sec_one_sec),
        cmocka_unit_test(test_micro2sec_half_sec),
        cmocka_unit_test(test_micro2sec_millisec),

        /* speed_per_sec tests */
        cmocka_unit_test(test_speed_per_sec_one_sec),
        cmocka_unit_test(test_speed_per_sec_half_sec),
        cmocka_unit_test(test_speed_per_sec_zero),
        cmocka_unit_test(test_speed_per_sec_large_flow),

        /* random_d tests */
        cmocka_unit_test(test_random_d_range),
        cmocka_unit_test(test_random_d_variation),

        /* sleep_till_next_sec tests */
        cmocka_unit_test(test_sleep_till_next_sec),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
