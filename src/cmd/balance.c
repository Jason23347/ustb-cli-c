#include "conf.h"

#include "cmd.h"

#include "calc/fee.h"
#include "calc/flow.h"
#include "lib/gstr.h"
#include "net/http.h"
#include "terminal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VAR_LEN 40

typedef struct info {
    uint64_t curr_flow;
    uint64_t curr_flow_v6;
    char ipv4_addr[16];
    char ipv6_addr[40];
    int ipv6_mode;
    unsigned fee;
} info_t;

static int
info_has_ipv6(const info_t *info) {
    int mode = info->ipv6_mode;
    return ((mode == 4) || (mode == 12));
}

static int
logged_in(const char *content) {
    return strstr(content, "uid=") != NULL;
}

int
info_fetch(info_t *info, const char *content) {
    int res;

    {
        const struct extract ext[1] = {{
            .dest = &info->curr_flow,
            .src = content,
            .fmt = &gstr_from_const(uint64_spec),
            .prefix = &gstr_from_const("flow"),
            .quoted = EXT_QUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    {
        const struct extract ext[1] = {{
            .dest = &info->curr_flow_v6,
            .src = content,
            .fmt = &gstr_from_const(uint64_spec),
            .prefix = &gstr_from_const("v6df"),
            .quoted = EXT_UNQUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    {
        const struct extract ext[1] = {{
            .dest = &info->ipv6_mode,
            .src = content,
            .fmt = &gstr_from_const("%u"),
            .prefix = &gstr_from_const("v46m"),
            .quoted = EXT_UNQUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    {
        const struct extract ext[1] = {{
            .dest = &info->fee,
            .src = content,
            .fmt = &gstr_from_const("%u"),
            .prefix = &gstr_from_const("fee"),
            .quoted = EXT_QUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    /* FIXME: Don't know why */
    info->curr_flow_v6 /= 4;

    {
        const struct extract ext[1] = {{
            .dest = &info->ipv4_addr,
            .src = content,
            .fmt = &gstr_from_const("%15[^']"),
            .prefix = &gstr_from_const("v4ip"),
            .quoted = EXT_QUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    {
        const struct extract ext[1] = {{
            .dest = &info->ipv6_addr,
            .src = content,
            .fmt = &gstr_from_const("%39[^']"),
            .prefix = &gstr_from_const("v6ip"),
            .quoted = EXT_QUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    return 0;
}

static void
info_print(const info_t *info) {
    char flow_str[32], flow_v6_str[32];

    flow_format(info->curr_flow, flow_str, sizeof(flow_str));
    flow_format(info->curr_flow_v6, flow_v6_str, sizeof(flow_v6_str));

    set_color(BLUE);
    printf("IPV4");
    reset_color();
    printf("\n");
    printf("IP Address:\t%.*s\n", (int)sizeof(info->ipv4_addr),
           info->ipv4_addr);
    printf("Flow used:\t%s\n", flow_str);
    uint64_t left = flow_left(info->curr_flow);
    flow_format(left, flow_str, sizeof(flow_str));
    printf("Flow left:\t%s\n", flow_str);

    printf("\n");

    if (!info_has_ipv6(info)) {
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

    int saving_rate =
        (100 * info->curr_flow_v6) / (info->curr_flow + info->curr_flow_v6);
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
        return -1;
    }

    const char *p = strstr(content, "<script");
    if (p == NULL) {
        return -1;
    }

    if (!logged_in(p)) {
        set_color(YELLOW);
        printf("Login required.\n");
        reset_color();
        return -1;
    }

    res = info_fetch(info, p);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    info_print(info);

    return EXIT_SUCCESS;
}

int
cmd_fee(int argc, char **argv) {
    int res;
    int c;
    uint64_t curr_flow;
    uint64_t fee_num;
    char fee_str[16];

    http_t *http = alloca(HTTP_T_SIZE);
    res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    const char *content = http_get_root(http);
    if (content == NULL) {
        return -1;
    }

    if (!logged_in(content)) {
        set_color(YELLOW);
        printf("Login required.\n");
        reset_color();
        return EXIT_FAILURE;
    }

    const char *p = strstr(content, "<script");
    if (p == NULL) {
        debug("failed to get variables: %s\n", content);
        return EXIT_FAILURE;
    }

    {
        const struct extract ext[1] = {{
            .dest = &curr_flow,
            .src = content,
            .fmt = &gstr_from_const(uint64_spec),
            .prefix = &gstr_from_const("flow"),
            .quoted = EXT_QUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    {
        const struct extract ext[1] = {{
            .dest = &fee_num,
            .src = content,
            .fmt = &gstr_from_const("%u"),
            .prefix = &gstr_from_const("fee"),
            .quoted = EXT_QUOTED,
        }};
        res = gstr_extract(ext);
        if (res < 0) {
            return -1;
        }
    }

    uint64_t over = flow_over(curr_flow);
    if (over <= 0) {
        fee_format(fee_str, sizeof(fee_str), fee_cost(curr_flow));
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
