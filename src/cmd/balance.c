#include "conf.h"

#include "cmd.h"

#include "calc/fee.h"
#include "calc/flow.h"
#include "lib/gbuff.h"
#include "net/http.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
info_print(const info_t *info) {
    char flow_str[32], flow_v6_str[32];

    flow_format(info->flow, flow_str, sizeof(flow_str));
    flow_format(info->flow_v6, flow_v6_str, sizeof(flow_v6_str));

    set_color(BLUE);
    printf("IPV4");
    reset_color();
    printf("\n");
    printf("IP Address:\t%.*s\n", (int)sizeof(info->ipv4_addr),
           info->ipv4_addr);
    printf("Flow used:\t%s\n", flow_str);
    uint64_t left = flow_left(info->flow);
    flow_format(left, flow_str, sizeof(flow_str));
    printf("Flow left:\t%s\n", flow_str);

    printf("\n");

    if (!has_ipv6(info)) {
        printf("IPV6 disabled\n");
    } else {
        set_color(GREEN);
        printf("IPV6");
        reset_color();
        printf("\n");
        printf("IP Address:\t%.*s\n", (int)sizeof(info->ipv6_addr),
               info->ipv6_addr);
        printf("Flow used:\t%s\n", flow_v6_str);
    }

    printf("\n");

    int saving_rate = (100 * info->flow_v6) / (info->flow + info->flow_v6);
    printf("Flow saving rate (%%): 0.%02d\n", saving_rate);

    printf("\n");
}

int
cmd_info(int argc, char **argv) {
    int res;
    info_t info[1] = {0};

    http_t *http = alloca(HTTP_T_SIZE);
    res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    const char *content = http_get_root(http);
    if (content == NULL) {
        return EXIT_FAILURE;
    }

    res = info_extract(info, content);
    if (res != USTB_OK) {
        if (!logged_in(info)) {
            set_color(YELLOW);
            printf("Login required.\n");
            reset_color();
        }
        return EXIT_FAILURE;
    }

    info_print(info);

    return EXIT_SUCCESS;
}

int
cmd_fee(int argc, char **argv) {
    int c;
    char fee_str[16];
    info_t info[1];

    http_t *http = alloca(HTTP_T_SIZE);
    int res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    const char *content = http_get_root(http);
    if (content == NULL) {
        return USTB_ERR;
    }

    res = info_extract(info, content);
    if (res != USTB_OK) {
        if (!logged_in(info)) {
            set_color(YELLOW);
            printf("Login required.\n");
            reset_color();
        }
        return EXIT_FAILURE;
    }

    uint64_t flow = info->flow;
    uint64_t fee_num = info->fee_num;

    if (flow_over(flow) > 0) {
        fee_format(fee_str, sizeof(fee_str), fee_cost(flow));
        c = cost_color(fee_str);
        printf("Money Cost: ");
        set_color(c);
        printf("￥%.*s", (int)sizeof(fee_str), fee_str);
        reset_color();
        printf("\n");
    } else {
        printf("Money Cost: ");
        set_color(GREEN);
        printf("￥0");
        reset_color();
        printf("\n");
    }

    fee_format(fee_str, sizeof(fee_str), fee_num);
    c = balance_color(fee_str);
    printf("Money Left: ");
    set_color(c);
    printf("￥%.*s", (int)sizeof(fee_str), fee_str);
    reset_color();
    printf("\n");

    printf("\n");

    return EXIT_SUCCESS;
}
