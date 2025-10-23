#include "conf.h"

#include "cmd.h"

#include <cargs.h>

#include <string.h>

extern const struct cag_option login_options[];
extern const size_t login_opt_count;
extern const struct cag_option speedtest_options[];
extern const size_t speedtest_opt_count;

extern const struct cmd_option commands[];
extern const size_t command_count;

extern const struct cag_option global_options[];
extern const size_t global_options_count;

static const char *
basename(const char *program) {
    const char *base = program;
    const char *slash = strrchr(base, '/');
    if (slash) {
        base = slash + 1;
    }

    return base;
}

static void
completion_zsh_print_commands(const char *var_name,
                              const struct cmd_option *commands, size_t len) {
    printf("  local -a %s=(\n", var_name);
    for (size_t i = 0; i < len; i++) {
        printf("    '%s:%s'\n", commands[i].name, commands[i].description);
    }
    printf("  )\n\n");
}

static int
cag_has_name(const cag_option *opt) {
    return (opt->access_name && opt->access_name[0]);
}

static int
cag_has_letters(const cag_option *opt) {
    return (opt->access_letters && opt->access_letters[0]);
}

static size_t
cag_letter_count(const cag_option *opt) {
    if (opt->access_letters != NULL) {
        return strlen(opt->access_letters);
    } else {
        return 0;
    }
}

static int
cag_has_value(const cag_option *opt) {
    return (opt->value_name != NULL);
}

static const char *
cag_description(const cag_option *opt) {
    if (opt->description != NULL && opt->description[0]) {
        return opt->description;
    } else {
        return "";
    }
}

static void
completion_zsh_print_opt_name(const cag_option *opt, int comma) {
    printf("--%s", opt->access_name);
    if (cag_has_value(opt)) {
        printf("=");
    }
    if (comma) {
        printf(",");
    } else {
        printf(" ");
    }
}

static void
completion_zsh_print_opt_letters(const cag_option *opt, int comma) {
    int has_value = cag_has_value(opt);
    size_t len = strlen(opt->access_letters);
    for (size_t i = 0; i < len - 1; i++) {
        char c = opt->access_letters[i];
        printf("-%c", c);
        if (has_value) {
            printf("=");
        }
        if (comma) {
            printf(",");
        } else {
            printf(" ");
        }
    }
    printf("-%c", opt->access_letters[len - 1]);
    if (has_value) {
        printf("=");
    }
}

static void
completion_zsh_print_opt_string(const cag_option *opt) {
    int has_letters = cag_has_letters(opt);
    int has_name = cag_has_name(opt);
    size_t letter_count = cag_letter_count(opt);
    const char *description = cag_description(opt);

    if (has_letters && has_name) {
        printf("    '(");
        completion_zsh_print_opt_name(opt, 0);
        completion_zsh_print_opt_letters(opt, 0);
        printf(")'");

        printf("{");
        completion_zsh_print_opt_name(opt, 1);
        completion_zsh_print_opt_letters(opt, 1);
        printf("}");

        printf("'[%s]'\n", description);
    } else if (has_letters) {
        if (letter_count <= 1) {
            printf("    '");
            completion_zsh_print_opt_letters(opt, 0);
            printf("[%s]'\n", description);
        } else {
            printf("    '(");
            completion_zsh_print_opt_letters(opt, 0);
            printf(")'");

            printf("{");
            completion_zsh_print_opt_letters(opt, 1);
            printf("}");

            printf("'[%s]'\n", description);
        }
    } else if (has_name) {
        printf("    '");
        completion_zsh_print_opt_name(opt, 0);
        printf("[%s]'\n", description);
    }
}

static void
completion_zsh_print_opts(const char *var_name,
                          const struct cag_option *options, size_t len) {
    printf("  local -a %s=(\n", var_name);
    for (size_t i = 0; i < len; i++) {
        const cag_option *opt = &options[i];
        completion_zsh_print_opt_string(opt);
    }
    printf("  )\n\n");
}

static void
completion_zsh_print_handler() {
    printf("  _arguments '1: :->cmds' '*::arg:->args'\n\n");
    printf("  case $state in\n"
           "    cmds)\n"
           "      _describe 'command' commands\n"
           "      ;;\n"
           "    args)\n"
           "      local command=$words[1]\n"
           "      opts_name=\"sub_opts_$command\"\n"
           "      if [ -v $opts_name ]; then\n"
           "        _arguments \"${global_opts[@]}\" \"${(@P)opts_name}\"\n"
           "      else\n"
           "        _arguments \"${global_opts[@]}\"\n"
           "      fi\n"
           "      ;;\n"
           "  esac\n");
}

static void
completion_zsh_print_function(const char *program) {
    printf("_%s() {\n", program);

    printf("  local curcontext=\"$curcontext\" state line\n");

    completion_zsh_print_commands("commands", commands, command_count);

    completion_zsh_print_opts("global_opts", global_options,
                              global_options_count);

    completion_zsh_print_opts("sub_opts_login", login_options, login_opt_count);
    completion_zsh_print_opts("sub_opts_speedtest", speedtest_options,
                              speedtest_opt_count);

    completion_zsh_print_handler();

    printf("}\n");
}

int
cmd_completion(int argc, char **argv) {
    const char *program = basename(argv[-1]);
    /* TODO other completions */
    const char *shell = (argc > 1) ? argv[1] : "zsh";

    if (strcmp(shell, "zsh") == 0) {
        printf("#compdef %s\n", program);
        completion_zsh_print_function(program);
        printf("compdef _%s %s\n", program, program);

        return 0;
    } else { /* Other shells */
        fprintf(stderr, "Unsupported shell: %s\n", shell);

        return 1;
    }
}
