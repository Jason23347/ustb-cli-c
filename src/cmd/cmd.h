#ifndef CMD_H
#define CMD_H

#include "config.h"

#include <cargs.h>
#include <stddef.h>
#include <stdint.h>

#define INFO_VAR_LEN 40

typedef struct info {
    // account
    char username[INFO_VAR_LEN];
    char nid[INFO_VAR_LEN];
    uint64_t fee_num;
    // flow & ip
    uint64_t flow;
    uint64_t flow_v6;
    char ipv4_addr[16];
    char ipv6_addr[40];
    int ipv6_mode;
} info_t;

typedef int USTB_RET;
static const USTB_RET USTB_OK = 0;
static const USTB_RET USTB_ERR = -1;

typedef int (*cmd_func_t)(int argc, char **argv);

struct cmd_option {
    const char name[16];
    const char description[32];
    cmd_func_t cmd_func;
    cmd_func_t cmd_help;
};

extern struct globconf {
    int need_help;
    enum LOG_LEVEL log_level;
#ifdef WITH_COLOR
    int raw_output; /* Output no color */
#endif
} global_config;

USTB_RET info_extract(info_t *info, const char *content);
int logged_in(const info_t *info);
int has_ipv6(const info_t *info);

int cmd_parse(int argc, char **argv);
// default
int cmd_completion(int argc, char **argv);
int cmd_help(int argc, char **argv);
int cmd_version(int argc, char **argv);
// balance
int cmd_info(int argc, char **argv);
int cmd_fee(int argc, char **argv);
// account
int cmd_login(int argc, char **argv);
int cmd_logout(int argc, char **argv);
int cmd_whoami(int argc, char **argv);
int cmd_devices(int argc, char **argv);
// speedtest
int cmd_speedtest(int argc, char **argv);
int cmd_monitor(int argc, char **argv);

int print_command_help(int argc, char **argv, const struct cag_option *cmd_opts,
                       size_t cmd_opt_count);

#endif /* CMD_H */
