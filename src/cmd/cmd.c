#include "conf.h"

#include "cmd.h"

#include "terminal.h"

#include <stdio.h>
#include <string.h>

extern const struct cmd_option *const cmd_options;
extern const size_t cmd_options_count;

const struct cmd_option _ustb_cmd_options[] = {
#ifdef WITH_ACCOUNT
    {
        .name = "login",
        .description = "Login to USTB web",
        .cmd_func = &cmd_login,
    },
#endif
#ifdef WITH_ACCOUNT
    {
        .name = "logout",
        .description = "Logout from USTB web",
        .cmd_func = &cmd_logout,
    },
#endif
#ifdef WITH_ACCOUNT
    {
        .name = "whoami",
        .description = "Show current user",
        .cmd_func = &cmd_whoami,
    },
#endif
#ifdef WITH_BALANCE
    {
        .name = "info",
        .description = "Show account info",
        .cmd_func = &cmd_info,
    },
#endif
#ifdef WITH_BALANCE
    {
        .name = "fee",
        .description = "Show money cost for this month",
        .cmd_func = &cmd_fee,
    },
#endif
#ifdef WITH_SPEEDTEST
    {
        .name = "speedtest",
        .description = "Test speed inside USTB web",
        .cmd_func = &cmd_speedtest,
    },
    {
        .name = "monitor",
        .description = "Monitor flow download speed",
        .cmd_func = &cmd_monitor,
    },
#endif
    {
        .name = "version",
        .description = "Print version and copyright",
        .cmd_func = &cmd_version,
    },
    {
        .name = "help",
        .description = "Print this help message",
        .cmd_func = &cmd_help,
    },
};
const struct cmd_option *const cmd_options = _ustb_cmd_options;
const size_t cmd_options_count =
    sizeof(_ustb_cmd_options) / sizeof(struct cmd_option);

int
cmd_help(int argc, char **argv) {
    printf("Usage: %s <command> [options]\n", argv[0]);
    printf("Commands:\n");
    for (size_t i = 0; i < cmd_options_count; i++) {
        printf("  %-10s %s\n", cmd_options[i].name, cmd_options[i].description);
    }

    return 0;
}

int
cmd_version(int argc, char **argv) {
    printf("%s %sv%s%s ", PACKAGE_NAME, color(YELLOW), PACKAGE_VERSION,
           color(NORMAL));
    printf(
#ifndef NDEBUG
        "(debug) "
#endif

#ifdef WITH_ACCOUNT
        "+account"
#endif

#ifdef WITH_BALANCE
        "+balance"
#endif

#ifdef WITH_COLOR
        "+color"
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

int
cmd_parse(int argc, char **argv) {
    for (size_t i = 0; i < cmd_options_count; i++) {
        if (strcmp(argv[1], cmd_options[i].name) == 0) {
            cmd_func_t func = cmd_options[i].cmd_func;
            return (*func)(argc - 1, argv + 1);
        }
    }

    // fallback to print usage info
    return cmd_help(argc, argv);
}
