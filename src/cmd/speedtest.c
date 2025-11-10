#include "conf.h"

#include "cmd.h"

#include "calc/flow.h"
#include "calc/timer.h"
#include "net/http.h"
#include "terminal.h"

#include <cargs.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define MAX_BUF_SIZE 4096

typedef struct speedtest {
    int test_upload;
    int test_download;
    int show_in_bits;
    size_t filesizeMB;
} speedtest_t;

const struct cag_option speedtest_options[] = {
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
        .description =
            "(NOT IMPLEMENTED) Specify NUM threads to use, default 4",
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

int
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
            return -1;
        }
    }

    return 0;
}

static suseconds_t
speedtest_download(const speedtest_t *config) {
    double r;
    int total;
    char buf[MAX_BUF_SIZE];
    struct timeval start, end;

    http_t *http = http_init(SPEEDTEST_DOMAIN, SPEEDTEST_PORT, IPV4_IPV6);

    gstr_t str[1] = {gstr_alloca(MAX_BUF_SIZE)};
    r = random_d();
    gstr_appendf(str, "%s?r=%lf&ckSize=%u", SPEEDTEST_DOWNLOAD_PATH, r,
                 config->filesizeMB);

    http_connect(http);
    http_send_request(http, str, NULL, NULL);
    http_section(http, buf, sizeof(buf));

    total = config->filesizeMB * ((MB * 1024) / sizeof(buf));

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
speedtest_upload(const speedtest_t *config) {
    double r;
    uint32_t seed;
    int total;
    char buf[MAX_BUF_SIZE];
    struct timeval start, end;

    http_t *http = http_init(SPEEDTEST_DOMAIN, SPEEDTEST_PORT, IPV4_IPV6);

    gstr_t str[1] = {gstr_alloca(MAX_BUF_SIZE)};
    r = random_d();
    gstr_appendf(str, "%s?r=%lf", SPEEDTEST_UPLOAD_PATH, r);

    http_connect(http);

    total = config->filesizeMB * ((1024 * 1024) / sizeof(buf));
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

int
cmd_speedtest(int argc, char **argv) {
    int res;
    uint64_t speed;
    suseconds_t interval;
    char speed_str[20];

    speedtest_t config[1] = {{
        .test_download = 0,
        .test_upload = 0,
        .show_in_bits = 0,
        .filesizeMB = 100,
    }};

    /* TODO 多线程下载/上传 */

    res = speedtest_get_config(config, argc, argv);
    if (res != 0) {
        return EXIT_FAILURE;
    }
    /* Default test both upload & download speed */
    if (config->test_upload == 0 && config->test_download == 0) {
        config->test_upload = 1;
        config->test_download = 1;
    }

    /* TODO ping */

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

static int
http_get_flow(http_t *http, uint64_t *flow) {
    assert(flow != NULL);

    int res;

    char *content = http_get_root(http);
    if (content == NULL) {
        return -1;
    }

    const struct extract ext[1] = {{
        .dest = &flow,
        .src = content,
        .fmt = &gstr_from_const(uint64_spec),
        .prefix = &gstr_from_const("flow"),
        .quoted = EXT_QUOTED,
    }};
    res = gstr_extract(ext);
    if (res < 0) {
        return -1;
    }

    free(content);

    return 0;
}

int
cmd_monitor(int argc, char **argv) {
    int res;
    char flow_str[20];
    uint64_t download, speed;
    flow_history_t history[1] = {0};

    for (; 1; sleep_till_next_sec()) {
        http_t *http = http_init(LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
        res = http_get_flow(http, &download);
        http_free(http);
        if (res != 0) {
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
