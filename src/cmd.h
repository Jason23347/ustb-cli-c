#ifndef CMD_H
#define CMD_H

#include <stddef.h>

typedef int (*cmd_func_t)(int argc, char **argv);

struct cmd_option {
    const char name[16];
    const char description[32];
    cmd_func_t cmd_func;
};

extern const struct cmd_option *const cmd_options;
extern const size_t cmd_options_count;

int cmd_parse(int argc, char **argv);
// default
int cmd_help(int argc, char **argv);
int cmd_version(int argc, char **argv);
// balance
int cmd_info(int argc, char **argv);
int cmd_fee(int argc, char **argv);
// account
int cmd_login(int argc, char **argv);
int cmd_logout(int argc, char **argv);
int cmd_whoami(int argc, char **argv);
// speedtest
int cmd_speedtest(int argc, char **argv);
int cmd_monitor(int argc, char **argv);

#endif /* CMD_H */
