#include "conf.h"

#include "cmd.h"

#include "calc/flow.h"
#include "calc/interval.h"
#include "net/http.h"
#include "terminal.h"
#include "thread.h"

#include <cargs.h>

#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define MAX_BUF_SIZE 4096

typedef struct speedtest {
    int test_ping;
    int test_upload;
    int test_download;
    int show_in_bits;
    size_t filesizeMB;
    int thread_count;
} speedtest_t;

typedef suseconds_t (*transfer_func_t)(size_t filesizeMB);

typedef struct transfer_thread_arg {
    size_t filesizeMB;
    suseconds_t *interval;
    int thread_id;
    transfer_func_t func;
} transfer_thread_arg_t;

typedef struct ping_result {
    double ping;   // 延迟 ms
    double jitter; // 抖动 ms
} ping_result_t;

const struct cag_option speedtest_options[] = {
    {
        .identifier = 'p',
        .access_letters = "p",
        .access_name = "ping",
        .value_name = NULL,
        .description = "Test ping latency",
    },
    {
        .identifier = 'c',
        .access_letters = "cs",
        .access_name = "file-size",
        .value_name = "NUM",
        .description = "Specify file size for each thread, default 100 in MB",
    },
    {
        .identifier = 'j',
        .access_letters = "j",
        .access_name = "thread",
        .value_name = "NUM",
        .description = "Specify NUM threads to use, default 4",
    },
    {
        .identifier = 'u',
        .access_letters = "u",
        .access_name = "upload",
        .value_name = NULL,
        .description = "Test upload speed, default to both",
    },
    {
        .identifier = 'd',
        .access_letters = "d",
        .access_name = "download",
        .value_name = NULL,
        .description = "Test download speed, default to both",
    },
    {
        .identifier = 'b',
        .access_letters = "b",
        .access_name = "bits",
        .value_name = NULL,
        .description = "Show speed in Mbps",
    },
};
const size_t speedtest_opt_count =
    sizeof(speedtest_options) / sizeof(speedtest_options[0]);

int
print_speedtest_help(int argc, char **argv) {
    return print_command_help(argc, argv, speedtest_options,
                              CAG_ARRAY_SIZE(speedtest_options));
}

static USTB_RET
speedtest_get_config(speedtest_t *config, int argc, char **argv) {
    const char *value;
    cag_option_context context;

    cag_option_init(&context, speedtest_options,
                    CAG_ARRAY_SIZE(speedtest_options), argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'c':
            value = cag_option_get_value(&context);
            if (value != NULL && strlen(value) != 0) {
                size_t size = atol(value);
                config->filesizeMB = size;
            }
            break;
        case 'j':
            value = cag_option_get_value(&context);
            if (value != NULL && strlen(value) != 0) {
                int threads = atoi(value);
                if (threads > 0)
                    config->thread_count = threads;
            }
            break;
        case 'p':
            config->test_ping = 1;
            break;
        case 'u':
            config->test_upload = 1;
            break;
        case 'd':
            config->test_download = 1;
            break;
        case 'b':
            config->show_in_bits = 1;
            break;
        case '?':
            cag_option_print_error(&context, stdout);
            print_speedtest_help(argc + 1, argv - 1);
            return USTB_ERR;
        }
    }

    return USTB_OK;
}

static suseconds_t
speedtest_download_single(size_t filesizeMB) {
    print_log(DEBUG, "Download thread: filesize %lu MB\n", filesizeMB);

    double r;
    int total;
    char buf[MAX_BUF_SIZE];
    struct timeval start, end;

    http_t *http = alloca(HTTP_T_SIZE);
    int res = http_init(http, SPEEDTEST_DOMAIN, SPEEDTEST_PORT, IPV4_IPV6);
    if (res != 0) {
        return 0;
    }

    gbuff_t str[1] = {gbuff_alloca(MAX_BUF_SIZE)};
    r = random_d();
    gbuff_appendf(str, "%s?r=%lf&ckSize=%u", SPEEDTEST_DOWNLOAD_PATH, r,
                  (unsigned int)filesizeMB);

    http_connect(http);
    http_send_request(http, str, NULL);
    http_section(http, buf, sizeof(buf));

    total = filesizeMB * ((MB * 1024) / sizeof(buf));

    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&start, NULL);
    for (int i = 0; i < total; i++) {
        http_read(http, buf, sizeof(buf));
    }
    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&end, NULL);

    http_close(http);

    return microsec_interval(start, end);
}

static void *
speedtest_download_thread(void *arg) {
    transfer_thread_arg_t *thread_arg = (transfer_thread_arg_t *)arg;
    suseconds_t interval = thread_arg->func(thread_arg->filesizeMB);
    *thread_arg->interval = interval;
    free(thread_arg);

    return NULL;
}

static suseconds_t
speedtest_transfer_concurrent(const speedtest_t *config, transfer_func_t func) {
    int thread_count = config->thread_count;
    size_t base_size = config->filesizeMB / thread_count;
    size_t remainder = config->filesizeMB % thread_count;

    ustb_thread_t *threads = malloc(sizeof(ustb_thread_t) * thread_count);
    suseconds_t *intervals = malloc(sizeof(suseconds_t) * thread_count);

    struct timeval global_start, global_end;

    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&global_start, NULL);

    for (int i = 0; i < thread_count; i++) {
        transfer_thread_arg_t *arg = malloc(sizeof(transfer_thread_arg_t));
        arg->filesizeMB = base_size;
        if ((remainder != 0) && (i == 0)) {
            arg->filesizeMB += remainder;
        }
        arg->interval = &intervals[i];
        arg->thread_id = i;
        arg->func = func;

        ustb_thread_create(&threads[i], speedtest_download_thread, arg);
    }

    for (int i = 0; i < thread_count; i++) {
        ustb_thread_join(threads[i]);
    }

    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&global_end, NULL);
    suseconds_t total_interval = microsec_interval(global_start, global_end);

    free(threads);
    free(intervals);

    return total_interval;
}

static suseconds_t
speedtest_download(const speedtest_t *config) {
    return speedtest_transfer_concurrent(config, speedtest_download_single);
}

static uint32_t
lcg_next(uint32_t *seed) {
    *seed = (*seed) * 1664525u + 1013904223u;
    return *seed;
}

static void
fill_random_with_seed(uint8_t *buf, size_t len, uint32_t *seed) {
    for (size_t i = 0; i < len; ++i) {
        buf[i] = (uint8_t)(lcg_next(seed) >> 24);
    }
}

static suseconds_t
speedtest_upload_single(size_t filesizeMB) {
    print_log(DEBUG, "Upload thread: filesize %lu MB\n", filesizeMB);

    double r;
    uint32_t seed;
    int total;
    char buf[MAX_BUF_SIZE];
    struct timeval start, end;

    http_t *http = alloca(HTTP_T_SIZE);
    int res = http_init(http, SPEEDTEST_DOMAIN, SPEEDTEST_PORT, IPV4_IPV6);
    if (res != 0) {
        return 0;
    }

    gbuff_t str[1] = {gbuff_alloca(MAX_BUF_SIZE)};
    r = random_d();
    gbuff_appendf(str, "%s?r=%lf", SPEEDTEST_UPLOAD_PATH, r);

    http_connect(http);

    total = filesizeMB * ((1024 * 1024) / sizeof(buf));
    seed = rand();

    fill_random_with_seed((uint8_t *)buf, sizeof(buf), &seed);

    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&start, NULL);
    for (int i = 0; i < total; i++) {
        http_write(http, buf, sizeof(buf));
    }
    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&end, NULL);

    http_close(http);

    return microsec_interval(start, end);
}

static suseconds_t
speedtest_upload(const speedtest_t *config) {
    return speedtest_transfer_concurrent(config, speedtest_upload_single);
}

static suseconds_t
http_ping_once(void) {
    struct timeval start, end;
    char resp[1];

    http_t *http = alloca(HTTP_T_SIZE);
    if (http_init(http, SPEEDTEST_DOMAIN, SPEEDTEST_PORT, IPV4_IPV6) != 0)
        return -1;

    if (http_connect(http) != 0) {
        http_close(http);
        return -1;
    }

    gbuff_t str[1] = {gbuff_alloca(MAX_BUF_SIZE)};
    double r = random_d();
    gbuff_appendf(str, "%s?r=%lf", SPEEDTEST_UPLOAD_PATH, r);

    http_send_request(http, str, NULL);

    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&start, NULL);
    if (http_read(http, resp, 1) <= 0) {
        http_close(http);
        return -1;
    }
    __asm__ __volatile__("" ::: "memory");
    gettimeofday(&end, NULL);

    http_close(http);

    return microsec_interval(start, end);
}

void
do_ping_test(ping_result_t *result, int count) {
    suseconds_t rtt;
    double inst_rtt = 0;
    double prev_rtt = 0;
    double min_ping = 0;
    double avg_jitter = 0;

    for (int i = 0; i < count; i++) {
        rtt = http_ping_once();
        if (rtt <= 0) {
            // 失败策略：这里选择跳过
            i--;
            continue;
        }

        inst_rtt = rtt / 1000.0; // 转成 ms

        // 防止出现0ms或异常小值
        if (inst_rtt < 1.0) {
            inst_rtt = (prev_rtt > 0) ? prev_rtt : 1.0;
        }

        if (i == 0) {
            // 第一次只记录，不参与统计
            prev_rtt = inst_rtt;
            continue;
        }

        if (i == 1) {
            min_ping = inst_rtt;
        } else {
            if (inst_rtt < min_ping)
                min_ping = inst_rtt;
        }

        // 计算瞬时抖动
        double inst_jitter = fabs(inst_rtt - prev_rtt);

        if (i == 2) {
            avg_jitter = inst_jitter; // 第一组有效抖动
        } else if (i > 2) {
            // 加权移动平均
            if (inst_jitter > avg_jitter) {
                avg_jitter = (avg_jitter * 0.3) + (inst_jitter * 0.7);
            } else {
                avg_jitter = (avg_jitter * 0.8) + (inst_jitter * 0.2);
            }
        }

        prev_rtt = inst_rtt;
    }

    result->ping = min_ping;
    result->jitter = avg_jitter;
}

int
cmd_speedtest(int argc, char **argv) {
    int res;
    uint64_t speed;
    suseconds_t interval;
    char speed_str[20];

    speedtest_t config[1] = {{
        .test_ping = 0,
        .test_download = 0,
        .test_upload = 0,
        .show_in_bits = 0,
        .filesizeMB = 100,
        .thread_count = 4,
    }};

    res = speedtest_get_config(config, argc, argv);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }
    /* Default test both upload & download speed */
    if (config->test_upload == 0 && config->test_download == 0) {
        config->test_upload = 1;
        config->test_download = 1;
    }

    /* Test ping */
    if (config->test_ping) {
        ping_result_t ping_result[1] = {0};
        do_ping_test(ping_result, SPEEDTEST_PING_TESTS);
        print_log(INFO, "Test %d times ping\n", SPEEDTEST_PING_TESTS);
        printf("Ping:   %.2f ms\n", ping_result->ping);
        printf("Jitter: %.2f ms\n", ping_result->jitter);
        printf("\n");
    }
    /* Show thread count */
    if (config->test_download || config->test_upload) {
        printf("Using %d threads\n", config->thread_count);
    }
    /* Download */
    if (config->test_download) {
        printf("Test download %lu MB\n", config->filesizeMB);
        interval = speedtest_download(config);
        speed = speed_per_sec(config->filesizeMB * MB, interval);
        flow_format_speed(speed, speed_str, sizeof(speed_str),
                          config->show_in_bits);
        printf("Elapsed time: %.2f s\n", micro2sec(interval));
        printf("Download speed: %.*s\n", (int)sizeof(speed_str), speed_str);
        printf("\n");
    }
    /* Upload */
    if (config->test_upload) {
        printf("Test upload %lu MB\n", config->filesizeMB);
        interval = speedtest_upload(config);
        speed = speed_per_sec(config->filesizeMB * MB, interval);
        flow_format_speed(speed, speed_str, sizeof(speed_str),
                          config->show_in_bits);
        printf("Elapsed time: %.2f s\n", micro2sec(interval));
        printf("Upload speed: %.*s\n", (int)sizeof(speed_str), speed_str);
        printf("\n");
    }

    return EXIT_SUCCESS;
}

static USTB_RET
http_get_flow(http_t *http, uint64_t *flow) {
    assert(flow != NULL);

    const char *content = http_get_root(http);
    if (content == NULL) {
        return USTB_ERR;
    }

    info_extract(flow, content, uint64_spec, "flow", EXT_QUOTED);

    return USTB_OK;
}

int
cmd_monitor(int argc, char **argv) {
    int res;
    char flow_str[20];
    uint64_t download, speed;
    flow_history_t history[1] = {0};

    http_t *http = alloca(HTTP_T_SIZE);
    res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    for (; 1; sleep_till_next_sec()) {
        res = http_get_flow(http, &download);
        if (res != USTB_OK) {
            set_color(RED);
            printf("ERROR");
            reset_color();

            continue;
        }

        speed = flow_speed(history, download);
        int c = flow_speed_color(speed);
        flow_format_speed(speed, flow_str, sizeof(flow_str), 0);

        /* TODO 下载量 & 下载速度，提供参数显示/隐藏 */
        clear_line();
        printf("  Download speed: "
               "[");
        set_color(c);
        printf("%s", flow_str);
        reset_color();
        printf("]\n");
        move_up_head();

        fflush(stdout);
    }

    return EXIT_SUCCESS;
}
