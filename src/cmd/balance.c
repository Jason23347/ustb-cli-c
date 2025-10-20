#include "conf.h"

#include "cmd.h"

#include "calc/fee.h"
#include "calc/flow.h"
#include "gstr.h"
#include "net/http.h"

#include <stdlib.h>
#include <string.h>

typedef struct info {
    uint64_t curr_flow;
    uint64_t curr_flow_v6;
    char ipv4_addr[16];
    char ipv6_addr[39];
    int ipv6_mode;
    unsigned fee;
} info_t;

static int
info_has_ipv6(const info_t *info) {
    int mode = info->ipv6_mode;
    return ((mode == 4) || (mode == 12));
}

int
info_fetch(info_t *info) {
    http_t http[1] = {{
        .domain = LOGIN_HOST,
        .port = LOGIN_PORT,
    }};

    if (http_get_root(http) == -1) {
        return -1;
    }

    const char *str, *p = strpos(http->buff, "<script");
    if (p == 0) {
        return -1;
    }

    strscan(str, "flow='", uint64_spec, info->curr_flow);
    strscan(str, "v6df=", uint64_spec, info->curr_flow_v6);
    strscan(str, "v46m=", "%u", info->ipv6_mode);
    strscan(str, "fee='", "%u", info->fee);

    info->curr_flow_v6 /= 4;

    char prefix_ip[] = "v4ip='";
    char suffix_ip = '\'';
    extract_between(info->ipv4_addr, p,           //
                    prefix_ip, sizeof(prefix_ip), //
                    suffix_ip,                    //
                    sizeof(info->ipv4_addr));

    // make it "v6ip='"
    prefix_ip[1] = '6';
    extract_between(info->ipv6_addr, p,           //
                    prefix_ip, sizeof(prefix_ip), //
                    suffix_ip,                    //
                    sizeof(info->ipv6_addr));

    return 0;
}

static void
info_print(const info_t *info) {
    char flow_str[32], flow_v6_str[32];

    flow_format(info->curr_flow, flow_str, sizeof(flow_str));
    flow_format(info->curr_flow_v6, flow_v6_str, sizeof(flow_v6_str));

    printf("IPV4\n");
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
        printf("IPV6\n");
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
    info_t info[1] = {0};

    info_fetch(info);
    info_print(info);

    return EXIT_SUCCESS;
}

int
cmd_fee(int argc, char **argv) {
    uint64_t curr_flow;
    uint64_t fee_num;
    char fee_str[16];

    http_t http[1] = {{
        .domain = LOGIN_HOST,
        .port = LOGIN_PORT,
    }};

    if (http_get_root(http) == -1) {
        return EXIT_FAILURE;
    }

    const char *str, *p = strpos(http->buff, "<script");
    if (p == 0) {
        debug("failed to get variables: %s\n", http->buff);
        return EXIT_FAILURE;
    }

    strscan(str, "flow='", uint64_spec, curr_flow);
    strscan(str, "fee='", uint64_spec, fee_num);

    uint64_t over = flow_over(curr_flow);
    if (over <= 0) {
        fee_format(fee_str, sizeof(fee_str), fee_cost(curr_flow));
        printf("Money Cost: ￥%.*s\n", (int)sizeof(fee_str), fee_str);
    } else {
        printf("Money Cost: ￥0\n");
    }

    fee_format(fee_str, sizeof(fee_str), fee_num);
    printf("Money Left: ￥%.*s\n", (int)sizeof(fee_str), fee_str);

    printf("\n");

    return EXIT_SUCCESS;
}
