#ifndef CMD_H
#define CMD_H

#include "config.h"

#include <cargs.h>
#include <stddef.h>

#define info_extract(destPtr, srcStr, fmtStr, prefixStr, extQuoted)            \
    do {                                                                       \
        const struct extract ext[1] = {{                                       \
            .dest = (destPtr),                                                 \
            .src = (srcStr),                                                   \
            .fmt = &gbuff_from_const(fmtStr),                                  \
            .prefix = &gbuff_from_const(prefixStr),                            \
            .quoted = extQuoted,                                               \
        }};                                                                    \
        int res = gbuff_extract(ext);                                          \
        if (res < 0) {                                                         \
            return USTB_ERR;                                                   \
        }                                                                      \
    } while (0)

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
