#include "conf.h"

#include "cmd.h"

#include "terminal.h"

#include <cargs.h>

#include <stdio.h>
#include <string.h>

extern int print_default_help(int argc, char **argv);
extern int print_login_help(int argc, char **argv);
extern int print_speedtest_help(int argc, char **argv);

extern struct globconf global_config;

const struct cmd_option commands[] = {
#ifdef WITH_ACCOUNT
    {
        .name = "login",
        .description = "Login to USTB web",
        .cmd_func = &cmd_login,
        .cmd_help = &print_login_help,
    },
#endif
#ifdef WITH_ACCOUNT
    {
        .name = "logout",
        .description = "Logout from USTB web",
        .cmd_func = &cmd_logout,
        .cmd_help = &print_default_help,
    },
#endif
#ifdef WITH_ACCOUNT
    {
        .name = "whoami",
        .description = "Show current user",
        .cmd_func = &cmd_whoami,
        .cmd_help = &print_default_help,
    },
#endif
#ifdef WITH_BALANCE
    {
        .name = "info",
        .description = "Show account info",
        .cmd_func = &cmd_info,
        .cmd_help = &print_default_help,
    },
#endif
#ifdef WITH_BALANCE
    {
        .name = "fee",
        .description = "Show money cost for this month",
        .cmd_func = &cmd_fee,
        .cmd_help = &print_default_help,
    },
#endif
#ifdef WITH_SPEEDTEST
    {
        .name = "speedtest",
        .description = "Test speed inside USTB web",
        .cmd_func = &cmd_speedtest,
        .cmd_help = &print_speedtest_help,
    },
    {
        .name = "monitor",
        .description = "Monitor flow download speed",
        .cmd_func = &cmd_monitor,
        .cmd_help = &print_default_help,
    },
#endif
    {
        .name = "version",
        .description = "Print version and copyright",
        .cmd_func = &cmd_version,
        .cmd_help = NULL,
    },
    {
        .name = "help",
        .description = "Print this help message",
        .cmd_func = &cmd_help,
        .cmd_help = NULL,
    },
};

const size_t command_count = sizeof(commands) / sizeof(commands[0]);

const struct cag_option global_options[] = {
    {
        .identifier = 'h',
        .access_letters = "h",
        .access_name = "help",
        .value_name = NULL,
        .description = "Print command help message",
    },
#ifdef WITH_COLOR
    {
        .identifier = 'r',
        .access_letters = "r",
        .access_name = "raw-output",
        .value_name = NULL,
        .description = "Print plain text instead of colorful output",
    },
#endif
};

struct globconf global_config = {
    .need_help = 0,
#ifdef WITH_COLOR
    .raw_output = 0,
#endif
};

int
cmd_help(int argc, char **argv) {
    printf("Usage: %s <command> [options]\n", argv[0]);
    printf("Commands:\n");
    for (size_t i = 0; i < command_count; i++) {
        printf("  %-10s %s\n", commands[i].name, commands[i].description);
    }
    printf("\nGlobal Options:\n");
    cag_option_print(global_options, CAG_ARRAY_SIZE(global_options), stdout);

    return 0;
}

int
print_command_help(int argc, char **argv, const struct cag_option *cmd_opts,
                   size_t cmd_opt_count) {
    const char *scriptname = argv[0];
    const char *command = argv[1];

    printf("Usage: %s %s [options]\n", scriptname, command);
    if (cmd_opts != NULL && cmd_opt_count > 0) {
        printf("Options:\n");
        cag_option_print(cmd_opts, cmd_opt_count, stdout);
    }
    printf("\nGlobal Options:\n");
    cag_option_print(global_options, CAG_ARRAY_SIZE(global_options), stdout);

    return 0;
}

int
print_default_help(int argc, char **argv) {
    return print_command_help(argc, argv, NULL, 0);
}

int
cmd_version(int argc, char **argv) {
    printf("%s ", PACKAGE_NAME);
    printf("v%s ", PACKAGE_VERSION);

#if defined(NDEBUG) && !defined(WITH_COLOR)
    /* Nothing */
#else
    printf("(");
#ifndef NDEBUG
    printf("debug");
#endif
#if !defined(NDEBUG) && defined(WITH_COLOR)
    printf(" ");
#endif
#ifdef WITH_COLOR
    set_color(YELLOW);
    printf("color");
    reset_color();
#endif
    printf(") ");
#endif

    printf(
#ifdef WITH_ACCOUNT
        "+account"
#endif

#ifdef WITH_BALANCE
        "+balance"
#endif

        "\n\n"
        "Copyright  2025     \tShuaicheng Zhu "
        "<jason23347@163.com>\n\n"
        "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF "
        "ANY KIND, "
        "EXPRESS OR\n"
        "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
        "MERCHANTABILITY,\n"
        "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. "
        "IN NO EVENT "
        "SHALL THE\n"
        "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, "
        "DAMAGES OR "
        "OTHER\n"
        "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR "
        "OTHERWISE, "
        "ARISING FROM,\n"
        "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR "
        "OTHER "
        "DEALINGS IN THE\n"
        "SOFTWARE."
        "\n\n");

    return 0;
}

static void
global_config_parse(int argc, char **argv) {
    cag_option_context context;
    cag_option_init(&context, global_options, CAG_ARRAY_SIZE(global_options),
                    argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
#ifdef WITH_COLOR
        case 'r':
            global_config.raw_output = 1;
            break;
#endif /* WITH_COLOR */
        case 'h':
            global_config.need_help = 1;
            break;
        case '?':
            /* Let them go */
            break;
        }
    }
}

int
cmd_parse(int argc, char **argv) {
    const char *command = argv[1];

    /* Shift args */
    argc--;
    argv++;

    global_config_parse(argc, argv);

    for (size_t i = 0; i < command_count; i++) {
        if (strcmp(command, commands[i].name) == 0) {
            cmd_func_t cmd_func = commands[i].cmd_func;
            cmd_func_t cmd_help = commands[i].cmd_help;
            if (global_config.need_help) {
                if (cmd_help != NULL) {
                    return (*cmd_help)(argc + 1, argv - 1);
                }
            } else {
                return (*cmd_func)(argc, argv);
            }
        }
    }

    // fallback to print usage info
    return cmd_help(argc + 1, argv - 1);
}
