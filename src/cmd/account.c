#include "conf.h"

#include "cmd.h"

#include "gstr.h"
#include "net/http.h"

#include <cargs.h>

#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LEN            128
#define MAX_VAR_LEN             40
#define MAX_PATH_LEN            200
#define URLENCODED_IPV6_MAX_LEN (40 + 7 * 2)

typedef struct login {
    int use_ipv6;
    const char *ipv6_addr;
    const char *env_filepath;
} login_t;

static struct cag_option login_options[] = {
    {
        .identifier = 'c',
        .access_letters = "c",
        .access_name = "env",
        .value_name = "FILEPATH",
        .description = "Specify env file path, default ~/" USTB_ENV_FILENAME,
    },
    {
        .identifier = 'i',
        .access_letters = "i6",
        .access_name = "enable-ipv6",
        .value_name = "USE_IPV6",
        .description = "Enable IPV6 or not, default true",
    },
};

int
print_login_help(int argc, char **argv) {
    return print_command_help(argc, argv, login_options,
                              CAG_ARRAY_SIZE(login_options));
}

static int
login_load_env(const char *env_filepath, char *username, char *password,
               size_t maxlen) {
    // Check if config->env_filepath exists.
    if (access(env_filepath, F_OK) != 0) {
        fprintf(stderr, "Env file does not exist: \"%s\"\n", env_filepath);
        return -1;
    }

    FILE *fp = fopen(env_filepath, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open env file: %s\n", env_filepath);
        return -1;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        // get USERNAME=xxx and PASSWORD=xxx
        if (0 == strncmp(line, USTB_USERNAME_VAR "=", USTB_USERNAME_LEN)) {
            snprintf(username, maxlen, "%.*s", (int)(maxlen - 1),
                     line + USTB_USERNAME_LEN);
            username[strcspn(username, "\r\n")] = '\0';
        } else if (0 ==
                   strncmp(line, USTB_PASSWORD_VAR "=", USTB_PASSWORD_LEN)) {
            snprintf(password, maxlen, "%.*s", (int)(maxlen - 1),
                     line + USTB_USERNAME_LEN);
            password[strcspn(password, "\r\n")] = '\0';
        }
    }
    fclose(fp);

    return 0;
}

// TODO 非常有优化空间
static size_t
ipv6_urlencode(char *dest, const char *ipv6_addr, size_t maxlen) {
    size_t di = 0;
    for (size_t si = 0;
         (ipv6_addr[si] != '\0') && (di < URLENCODED_IPV6_MAX_LEN); si++) {
        if (ipv6_addr[si] == ':') {
            dest[di++] = '%';
            dest[di++] = '3';
            dest[di++] = 'A';
        } else {
            dest[di++] = ipv6_addr[si];
        }
    }
    dest[di] = '\0';
    return di;
}

static int
login_url_path(const login_t *config, gstr_t *str) {
    char username[MAX_VAR_LEN] = "";
    char password[MAX_VAR_LEN] = "";

    // Get username & password
    int res =
        login_load_env(config->env_filepath, username, password, MAX_VAR_LEN);
    if (res != 0) {
        return -1;
    }

    if (username[0] == '\0' || password[0] == '\0') {
        fprintf(stderr, USTB_USERNAME_VAR " or " USTB_PASSWORD_VAR
                                          " not found in env file\n");
        return -1;
    }

    gstr_appendf(str, LOGIN_PATH "?callback=a&DDDDD=%s&upass=%s&0MKKey=123456",
                 username, password);
    if (config->use_ipv6) {
        char ipv6_encoded[URLENCODED_IPV6_MAX_LEN];
        ipv6_urlencode(ipv6_encoded, config->ipv6_addr, sizeof(ipv6_encoded));
        gstr_appendf(str, "&v6ip=%.*s", (int)sizeof(ipv6_encoded),
                     ipv6_encoded);
    }

    return 0;
}

/* Default to ~/.ustb.env */
static int
get_defule_env_path(gstr_t *home_str) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }

    if (!home) {
        return -1;
    }

    gstr_appendf(home_str, "%s/" USTB_ENV_FILENAME, home);

    return 0;
}

int
login_get_config(login_t *config, int argc, char **argv) {
    const char *value;
    cag_option_context context;

    config->use_ipv6 = 1;

    cag_option_init(&context, login_options, CAG_ARRAY_SIZE(login_options),
                    argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'c':
            value = cag_option_get_value(&context);
            if (value != NULL && strlen(value) != 0) {
                config->env_filepath = value;
            }
            break;
        case 'i':
            value = cag_option_get_value(&context);
            if (value != NULL && strlen(value) != 0) {
                const char str_true[] = "true";
                int cmp = strncmp(value, str_true, sizeof(str_true));
                config->use_ipv6 = (cmp == 0);
            }
            break;
        case '?':
            cag_option_print_error(&context, stdout);
            print_login_help(argc + 1, argv - 1);
            return -1;
        }
    }

    return 0;
}

static int
ipv6_get(char *ipv6_addr, size_t maxlen) {
    http_t http[1] = {{
        .domain = CIPPV6_DOMAIN,
        .port = CIPPV6_PORT,
    }};

    int res = http_get(http, &gstr_from_const(CIPPV6_PATH));
    if (res != 0) {
        // failed to get ipv6
        return -1;
    } else {
        // Extract ipv6_addr between single-quotes
        char *start = strchr(http->buff, '\'');
        if (!start) {
            return -1;
        }
        char *end = strchr(start + 1, '\'');
        if (!end || end == start + 1) {
            return -1;
        }
        size_t len = end - (start + 1);
        if (len > maxlen) {
            len = maxlen;
        }
        strncpy(ipv6_addr, start + 1, len);
    }

    return 0;
}

static int
login_request(const gstr_t *path) {
    http_t http[1] = {{
        .domain = LOGIN_HOST,
        .port = LOGIN_PORT,
    }};

    int res = http_get(http, path);
    if (res != 0) {
        return -1;
    }

    if (strstr(http->buff, "\"result\":1") == NULL) {
        return -1;
    }

    debug("%s\n", http->buff);

    return 0;
}

int
cmd_login(int argc, char **argv) {
    int res;
    char ipv6_buf[40];
    char filepath_buf[MAX_PATH_LEN];
    login_t config[1] = {0};

    // Get username & password & other things
    res = login_get_config(config, argc, argv);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    // Fix env filepath
    if (config->env_filepath == NULL) {
        // fallback to default env
        gstr_t home_str[1] = {gstr_from_buf(filepath_buf)};
        int res = get_defule_env_path(home_str);
        if (res != 0) {
            return -1;
        }
        config->env_filepath = home_str->s;
    }

    // Get IPV6 address
    if (config->use_ipv6) {
        printf("Fetching IPV6 address...");
        res = ipv6_get(ipv6_buf, sizeof(ipv6_buf));
        if (res != 0) {
            printf("failed");
            config->use_ipv6 = 0;
        } else {
            config->ipv6_addr = ipv6_buf;
            printf("%.*s", (int)sizeof(ipv6_buf), ipv6_buf);
        }
    }
    printf("\n");

    // Assemble URL path
    char buf[MAX_PATH_LEN] = {0};
    gstr_t path[1] = {gstr_from_buf(buf)};
    res = login_url_path(config, path);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    // Send request
    res = login_request(path);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
cmd_logout(int argc, char **argv) {
    http_t http[1] = {{
        .domain = LOGIN_HOST,
        .port = LOGIN_PORT,
    }};

    int res = http_get(http, &gstr_from_const("/F.htm"));
    if (res != 0) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
cmd_whoami(int argc, char **argv) {
    int res;

    http_t http[1] = {{
        .domain = LOGIN_HOST,
        .port = LOGIN_PORT,
    }};

    res = http_get_root(http);
    if (res == -1) {
        return EXIT_FAILURE;
    }

    char username[MAX_VAR_LEN];
    res = extract(username, http->buff, "%[^']s", "uid", 1);
    if (res < 0) {
        return EXIT_FAILURE;
    }

    printf("%s", username);

    return EXIT_SUCCESS;
}
