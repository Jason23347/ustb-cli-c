#include "conf.h"

#include "cmd.h"

#include "calc/flow.h"
#include "calc/timer.h"
#include "net/http.h"
#include "terminal.h"

#include <assert.h>
#include <stdlib.h>

int
cmd_speedtest(int argc, char **argv) {
    // TODO speedtest
    return 0;
}

static int
http_get_flow(http_t *http, uint64_t *flow) {
    assert(flow != NULL);

    int res;

    res = http_get_root(http);
    if (res != 0) {
        return -1;
    }

    extract(flow, http->buff, uint64_spec, "flow", 1);

    http_free(http);

    return 0;
}

int
cmd_monitor(int argc, char **argv) {
    int res;
    char flow_str[20];
    uint64_t download, speed;
    flow_history_t history[1] = {0};
    http_t http[1] = {{
        .domain = LOGIN_HOST,
        .port = LOGIN_PORT,
    }};

    for (; 1; sleep_till_next_sec()) {
        res = http_get_flow(http, &download);
        if (res != 0) {
            set_color(RED);
            printf("ERROR");
            reset_color();

            continue;
        }

        speed = flow_speed(history, download);
        int c = flow_speed_color(speed);
        flow_format_speed(speed, flow_str, sizeof(flow_str));

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
