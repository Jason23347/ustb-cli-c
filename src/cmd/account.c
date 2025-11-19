#include "conf.h"

#include "cmd.h"

#include "lib/decode.h"
#include "lib/gbuff.h"
#include "lib/hash.h"
#include "net/http.h"
#include "terminal.h"

#include <cargs.h>

#include <iconv.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_LEN            128
#define MAX_VAR_LEN             40
#define MAX_PATH_LEN            200
#define URLENCODED_IPV6_MAX_LEN (40 + 7 * 2)
#define MAX_ONLINE_DEVICE_COUNT 4
#define MAC_HEX_LEN             12
#define MAC_FORMATTED_LEN       17

#define min(a, b) (((a) < (b)) ? (a) : (b))

typedef struct login {
    int use_ipv6;
    const char *ipv6_addr;
    const char *env_filepath;
} login_t;

typedef struct {
    gbuff_t username[1];
    gbuff_t password[1];
} account_t;

typedef struct device_info {
    char ipv4_addr[16];
    char ipv6_addr[40];
    char mac[MAC_FORMATTED_LEN + 1];
} device_info_t;

typedef struct device {
    int watch_mode;
    int output_markdown;
    int with_separator;
    int watch_seconds;
    const char *env_filepath;
} device_t;

typedef struct {
    char *account;
    char pw_hash[MD5_LEN];
    char checkcode[5];
    char *submit;
    int trytimes;
} device_form_t;

typedef struct whoami {
    enum {
        PRINT_UNAME = 0,
#define PRINT_DEFAULT PRINT_UNAME
        PRINT_ALL = 1,
        PRINT_NID = 2,
    } mode;
} whoami_t;

const struct cag_option login_options[] = {
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
const size_t login_opt_count = CAG_ARRAY_SIZE(login_options);

const struct cag_option whoami_options[] = {
    {
        .identifier = 'a',
        .access_letters = "a",
        .access_name = NULL,
        .value_name = NULL,
        .description = "Print full information",
    },
    {
        .identifier = 'N',
        .access_letters = "N",
        .access_name = NULL,
        .value_name = NULL,
        .description = "Print NID",
    },
};
const size_t whoami_opt_count = CAG_ARRAY_SIZE(whoami_options);

const struct cag_option devices_options[] = {
    {
        .identifier = 'c',
        .access_letters = "c",
        .access_name = "env",
        .value_name = "FILEPATH",
        .description = "Specify env file path, default ~/" USTB_ENV_FILENAME,
    },
    {
        .identifier = 'm',
        .access_letters = "m",
        .access_name = "output-markdown",
        .value_name = NULL,
        .description = "Output results in Markdown table format",
    },
    {
        .identifier = 'f',
        .access_letters = "f",
        .access_name = "with-separator",
        .value_name = NULL,
        .description = "Display MAC addresses with separators",
    },
    {
        .identifier = 'w',
        .access_letters = "w",
        .access_name = "watch",
        .value_name = NULL,
        .description = "Continuously watch for changes",
    },
    {
        .identifier = 't',
        .access_letters = "t",
        .access_name = NULL,
        .value_name = "SECONDS",
        .description = "Set the refresh interval for watch mode",
    },
};
const size_t devices_opt_count = CAG_ARRAY_SIZE(devices_options);

int
print_login_help(int argc, char **argv) {
    return print_command_help(argc, argv, login_options,
                              CAG_ARRAY_SIZE(login_options));
}

int
print_whoami_help(int argc, char **argv) {
    return print_command_help(argc, argv, whoami_options,
                              CAG_ARRAY_SIZE(whoami_options));
}

int
print_devices_help(int argc, char **argv) {
    return print_command_help(argc, argv, devices_options,
                              CAG_ARRAY_SIZE(devices_options));
}

static USTB_RET
account_load_env(account_t *account, const char *env_filepath) {
    // Check if config->env_filepath exists.
    if (access(env_filepath, F_OK) != 0) {
        fprintf(stderr, "Env file does not exist: \"%s\"\n", env_filepath);
        return USTB_ERR;
    }

    FILE *fp = fopen(env_filepath, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open env file: %s\n", env_filepath);
        return USTB_ERR;
    }

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), fp)) {
        // get USERNAME=xxx and PASSWORD=xxx
        if (0 == strncmp(line, USTB_USERNAME_VAR "=", USTB_USERNAME_LEN)) {
            gbuff_t *username = account->username;
            size_t offset = strcspn(line, "\r\n");
            line[offset] = '\0';
            gbuff_appendf(username, "%s", line + USTB_USERNAME_LEN);
        } else if (0 ==
                   strncmp(line, USTB_PASSWORD_VAR "=", USTB_PASSWORD_LEN)) {
            gbuff_t *password = account->password;
            size_t offset = strcspn(line, "\r\n");
            line[offset] = '\0';
            gbuff_appendf(password, "%s", line + USTB_USERNAME_LEN);
        }
    }
    fclose(fp);

    return USTB_OK;
}

// TODO 非常有优化空间
static void
ipv6_urlencode(gbuff_t *dest, const char *ipv6_addr, size_t maxlen) {
    const char colon = ':';
    const char colon_encoded[] = "%3A";
    for (const char *p = ipv6_addr; *p != '\0'; p++) {
        if (*p == colon) {
            gbuff_appendf(dest, "%s", colon_encoded);
        } else {
            gbuff_appendf(dest, "%c", *p);
        }
    }
    gbuff_appendf(dest, "");
}

static USTB_RET
login_url_path(const login_t *config, gbuff_t *str) {
    account_t account[1] = {{
        .username = {gbuff_alloca(MAX_VAR_LEN)},
        .password = {gbuff_alloca(MAX_VAR_LEN)},
    }};

    // Get username & password
    int res = account_load_env(account, config->env_filepath);
    if (res != USTB_OK) {
        return USTB_ERR;
    }

    if ((account->username->len == 0) || (account->password->len == 0)) {
        fprintf(stderr, USTB_USERNAME_VAR " or " USTB_PASSWORD_VAR
                                          " not found in env file\n");
        return USTB_ERR;
    }

    /* The order matters */
    gbuff_appendf(str, LOGIN_PATH "?callback=a&DDDDD=");
    gbuff_concat(str, account->username);
    gbuff_appendf(str, "&upass=");
    gbuff_concat(str, account->password);
    gbuff_appendf(str, "&0MKKey=123456");

    if (config->use_ipv6) {
        gbuff_t *ipv6_encoded = &gbuff_alloca(URLENCODED_IPV6_MAX_LEN);
        ipv6_urlencode(ipv6_encoded, config->ipv6_addr, sizeof(ipv6_encoded));
        gbuff_appendf(str, "&v6ip=");
        gbuff_concat(str, ipv6_encoded);
    }

    return USTB_OK;
}

/* Default to ~/.ustb.env */
static USTB_RET
get_defule_env_path(gbuff_t *home_str) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            home = pw->pw_dir;
        }
    }

    if (!home) {
        return USTB_ERR;
    }

    gbuff_appendf(home_str, "%s/" USTB_ENV_FILENAME, home);

    return USTB_OK;
}

static USTB_RET
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
            return USTB_ERR;
        }
    }

    return USTB_OK;
}

static USTB_RET
ipv6_get(char *ipv6_addr, size_t maxlen) {
    int res;
    http_t *http = alloca(HTTP_T_SIZE);
    res = http_init(http, CIPPV6_DOMAIN, CIPPV6_PORT, IPV6_ONLY);
    if (res != 0) {
        return USTB_ERR;
    }

    const char *content = http_get(http, &gbuff_from_const(CIPPV6_PATH));
    if (content == NULL) {
        // failed to get ipv6
        return USTB_ERR;
    }

    // Extract ipv6_addr between single-quotes
    const char *p = strchr(content, '\'');
    sscanf(p + 1, "%39[^']s", ipv6_addr);

    return USTB_OK;
}

static USTB_RET
login_request(const gbuff_t *path) {
    http_t *http = alloca(HTTP_T_SIZE);
    int res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return USTB_ERR;
    }

    const char *content = http_get(http, path);
    if (content == NULL) {
        return USTB_ERR;
    }

    if (strstr(content, "\"result\":1") == NULL) {
        return USTB_ERR;
    }

    print_log(DEBUG, "%s\n", content);

    return USTB_OK;
}

int
cmd_login(int argc, char **argv) {
    int res;
    char ipv6_buf[40];
    login_t config[1] = {0};

    // Get username & password & other things
    res = login_get_config(config, argc, argv);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    // Fix env filepath
    if (config->env_filepath == NULL) {
        gbuff_t home_str[1] = {gbuff_alloca(MAX_PATH_LEN)};
        // fallback to default env
        res = get_defule_env_path(home_str);
        if (res != USTB_OK) {
            return USTB_ERR;
        }
        config->env_filepath = home_str->data;
    }

    // Get IPV6 address
    if (config->use_ipv6) {
        printf("Fetching IPV6 address...");
        res = ipv6_get(ipv6_buf, sizeof(ipv6_buf));
        if (res != USTB_OK) {
            printf("failed");
            config->use_ipv6 = 0;
        } else {
            config->ipv6_addr = ipv6_buf;
            printf("%.*s", (int)sizeof(ipv6_buf), ipv6_buf);
        }
    }
    printf("\n");

    // Assemble URL path
    gbuff_t path[1] = {gbuff_alloca(MAX_PATH_LEN)};
    res = login_url_path(config, path);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    // Send request
    res = login_request(path);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int
cmd_logout(int argc, char **argv) {
    http_t *http = alloca(HTTP_T_SIZE);
    int res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    const char *content = http_get(http, &gbuff_from_const("/F.htm"));
    if (content == NULL) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static USTB_RET
whoami_get_config(whoami_t *config, int argc, char **argv) {
    cag_option_context context;

    cag_option_init(&context, whoami_options, CAG_ARRAY_SIZE(whoami_options),
                    argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'N':
            config->mode = PRINT_NID;
            break;
        case 'a':
            config->mode = PRINT_ALL;
            break;
        case '?':
            cag_option_print_error(&context, stdout);
            print_whoami_help(argc + 1, argv - 1);
            return USTB_ERR;
        }
    }

    return USTB_OK;
}

int
cmd_whoami(int argc, char **argv) {
    int res;

    whoami_t config[1] = {{
        .mode = PRINT_DEFAULT,
    }};

    res = whoami_get_config(config, argc, argv);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    http_t *http = alloca(HTTP_T_SIZE);
    res = http_init(http, LOGIN_HOST, LOGIN_PORT, IPV4_ONLY);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    const char *content = http_get_root(http);
    if (content == NULL) {
        return EXIT_FAILURE;
    }

    char username[MAX_VAR_LEN] = {0};
    info_extract(username, content, "%[^'\"]s", "uid", EXT_QUOTED);

    char nid[MAX_VAR_LEN] = {0};
    info_extract(nid, content, "%[^'\"]s", "NID", EXT_QUOTED);

    /* GBK → UTF-8 */
    gbuff_t nid_str[1] = {{
        .data = nid,
        .len = strlen(nid),
        .cap = sizeof(nid) - 1,
    }};
    size_t nid_buf_size = strlen(nid) * 3 / 2 + 1;

    if (config->mode == PRINT_UNAME) {
        printf("%s", username);
    } else if (config->mode == PRINT_NID) {
        printf("%s", nid);
    } else if (config->mode == PRINT_ALL) {
        gbuff_t nid_utf8[1] = {gbuff_alloca(nid_buf_size)};
        gbuff_clear(nid_utf8);
        decode_gb2312(nid_utf8, nid_str);
        printf("%s (%s)\n", username, nid_utf8->data);
    } else {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

static int
extract_trytimes(const char *content) {
    int res;
    char buf[MAX_VAR_LEN];
    const struct extract ext[1] = {{
        .dest = buf,
        .src = content,
        .fmt = &gbuff_from_const("%[^'\"]"),
        .prefix = &gbuff_from_const("trytimes"),
        .quoted = EXT_QUOTED,
    }};
    res = gbuff_extract(ext);
    if (res < 0) {
        return 0;
    } else if (strcmp(buf, "null") == 0) {
        return 0;
    } else { /* 假设 trytimes < 10 */
        return buf[0] - '0';
    }
}

static USTB_RET
device_get_form(device_form_t *form, http_t *http, const account_t *account) {
    const char *content =
        http_request(http, &gbuff_from_const(DRCOM_FORM_PATH), NULL);

    info_extract(form->checkcode, content, "%[^'\"]", "checkcode", EXT_QUOTED);

    form->trytimes = extract_trytimes(content);

    if (form->trytimes >= 3) {
        return USTB_ERR;
    }

    form->account = account->username->data;
    md5(form->pw_hash, account->password->data);
    /* 登[空格]录 */
    form->submit = "\%E7\%99\%BB+\%E5\%BD\%95";

    return USTB_OK;
}

static char *
step_in(const char *p, const char *tag_name) {
    char *pos;
    const char br_left = '<', br_right = '>';

    gbuff_t tag[1] = {gbuff_alloca(strlen(tag_name) + 2)};
    gbuff_appendf(tag, "%c%s", br_left, tag_name);

    /* <tag_name ...> */
    pos = strstr(p, tag->data);
    if (pos == NULL) {
        return NULL;
    }
    pos = strchr(pos, br_right);
    return (pos + 1);
}

static char *
step_out(const char *p, const char *tag_name) {
    char *pos;
    const char br_left = '<', br_right = '>';

    gbuff_t tag[1] = {gbuff_alloca(strlen(tag_name) + 4)};
    gbuff_appendf(tag, "%c/%s%c", br_left, tag_name, br_right);

    /* </tag_name ...> */
    pos = strstr(p, tag->data);
    if (pos == NULL) {
        return NULL;
    }
    return (pos + tag->len);
}

static ssize_t
devices_parse(device_info_t *devices, const char *content, size_t count) {
    size_t i = 0;
    const char *p = content;

    p = step_in(p, "tbody");
    if (p == NULL) {
        return -1;
    }

    for (i = 0; i < count; i++) {
        const char *h1, *h2, *h3;
        const char *t1, *t2, *t3;

        device_info_t *device = &devices[i];

        p = step_in(p, "tr");
        if (p == NULL) {
            break;
        }

        /* 1st col: IPV4 address */
        p = step_in(p, "td");
        if (p == NULL) {
            return -1;
        }
        h1 = p;
        p = step_out(p, "td");
        if (p == NULL) {
            return -1;
        }

        /* 2nd col: IPV6 address */
        p = step_in(p, "td");
        if (p == NULL) {
            return -1;
        }
        h2 = p;
        p = step_out(p, "td");
        if (p == NULL) {
            return -1;
        }

        /* 3rd col: MAC address */
        p = step_in(p, "td");
        if (p == NULL) {
            return -1;
        }
        h3 = p;
        p = step_out(p, "td");
        if (p == NULL) {
            return -1;
        }

        p = step_out(p, "tr");
        if (p == NULL) {
            return -1;
        }

        /* 1st col: IPV4 address */
        t1 = strchr(h1, '&');
        if (t1 == NULL) {
            const char stop[] = " <\r\n";
            t1 = strpbrk(h1, stop);
            if (t1 == NULL) {
                return -1;
            }
        }

        size_t len = t1 - h1;
        snprintf(device->ipv4_addr, min(len + 1, sizeof(device->ipv4_addr) - 1),
                 "%s", h1);

        /* 2nd col: IPV6 address */
        t2 = strchr(h2, '&');
        if (t2 == NULL) {
            const char stop[] = " <\r\n";
            t2 = strpbrk(h2, stop);
            if (t2 == NULL) {
                return -1;
            }
        }

        len = t2 - h2;
        snprintf(device->ipv6_addr, min(len + 1, sizeof(device->ipv6_addr) - 1),
                 "%s", h2);

        /* 3rd col: MAC address */
        t3 = strchr(h3, '&');
        if (t3 == NULL) {
            const char stop[] = " <\r\n";
            t3 = strpbrk(h3, stop);
            if (t3 == NULL) {
                return -1;
            }
        }

        len = t3 - h3;
        snprintf(device->mac, min(len + 1, sizeof(device->mac) - 1), "%s", h3);
    }

    return i;
}

static USTB_RET
devices_get_config(device_t *config, int argc, char **argv) {
    const char *value;
    cag_option_context context;

    cag_option_init(&context, devices_options, CAG_ARRAY_SIZE(devices_options),
                    argc, argv);
    while (cag_option_fetch(&context)) {
        switch (cag_option_get_identifier(&context)) {
        case 'c':
            value = cag_option_get_value(&context);
            if (value != NULL && strlen(value) != 0) {
                config->env_filepath = value;
            }
            break;
        case 'm':
            config->output_markdown = 1;
            break;
        case 'f':
            config->with_separator = 1;
            break;
        case 'w':
            config->watch_mode = 1;
            break;
        case 't':
            value = cag_option_get_value(&context);
            if (value != NULL && strlen(value) != 0) {
                config->watch_seconds = atoi(value);
            }
            break;
        case '?':
            cag_option_print_error(&context, stdout);
            print_whoami_help(argc + 1, argv - 1);
            return USTB_ERR;
        }
    }

    if (config->watch_mode) {
        if (config->watch_seconds <= 0) {
            config->watch_seconds = 3;
        }
    }

    return USTB_OK;
}

static USTB_RET
devices_login(http_t *http, const account_t *account) {
    const char *content;
    device_form_t form[1];

    // 1. Get checkcode & trytime
    content = http_request(http, &gbuff_from_const(DRCOM_FORM_PATH), NULL);
    if (content == NULL) {
        return USTB_ERR;
    }
    device_get_form(form, http, account);

    // 2. Get random code (never use)
    content =
        http_request(http, &gbuff_from_const(DRCOM_RANDOMCODE_PATH), NULL);
    if (content == NULL) {
        return USTB_ERR;
    }

    // 3. login request
    gbuff_t data[1] = {gbuff_alloca(MAX_PATH_LEN)};
    gbuff_appendf(data, "account=%s&password=%s&code=&checkcode=%s&Submit=%s",
                  form->account, form->pw_hash, form->checkcode, form->submit);
    content = http_request(http, &gbuff_from_const(DRCOM_LOGIN_PATH), data);

    return USTB_OK;
}

static USTB_RET
devices_check_online(device_info_t *devices, http_t *http, size_t maxlen) {
    const char *content =
        http_request(http, &gbuff_from_const(DRCOM_DEVICES_PATH), NULL);
    int device_count = devices_parse(devices, content, maxlen);
    if (device_count < 0) {
        /* Maybe login failed */
        return USTB_ERR;
    }

    return device_count;
}

static void
devices_format_mac(device_info_t *devices, size_t device_count) {
    char hex[MAC_HEX_LEN + 1];

    for (size_t i = 0; i < device_count; i++) {
        device_info_t *device = &devices[i];

        assert(strlen(device->mac) == MAC_HEX_LEN);

        memcpy(hex, device->mac, sizeof(hex));
        snprintf(device->mac, sizeof(device->mac),
                 "%c%c:%c%c:%c%c:%c%c:%c%c:%c%c", hex[0], hex[1], hex[2],
                 hex[3], hex[4], hex[5], hex[6], hex[7], hex[8], hex[9],
                 hex[10], hex[11]);
    }
}

static void
devices_output(device_t *config, device_info_t *devices, size_t device_count) {
    const char *header_format;
    const char *body_format;

    if (config->output_markdown) {
        header_format = "| %-15s | %-39s | %-17s |\n| :-------------- | "
                        ":-------------------------------------- | "
                        ":---------------- |\n";
        body_format = "| %-15s | %-39s | %-17s |\n";
    } else {
        header_format = "%-15s %-39s %s\n";
        body_format = "%-15s %-39s %-17s\n";
    }

    if (config->watch_mode) {
        time_t t;
        time(&t);
        printf("  %s", ctime(&t));
    }

    // Check MAC format
    if (config->with_separator) {
        devices_format_mac(devices, device_count);
    }

    printf(header_format, "IPV4 Address", "IPV6 Address", "MAC");
    for (size_t i = 0; i < device_count; i++) {
        device_info_t device = devices[i];
        printf(body_format, device.ipv4_addr, device.ipv6_addr, device.mac);
    }
    for (size_t i = device_count; i < MAX_ONLINE_DEVICE_COUNT; i++) {
        for (int j = 0; j < 82; j++) {
            printf(" ");
        }
        printf("\n");
    }

    if (config->watch_mode) {
        for (size_t i = 0; i < MAX_ONLINE_DEVICE_COUNT; i++) {
            move_up_head();
        }
        /* header line */
        move_up_head();
        /* Time line */
        move_up_head();

        fflush(stdout);
    }
}

int
cmd_devices(int argc, char **argv) {
    int res;

    device_t config[1] = {{
        .env_filepath = NULL,
        .output_markdown = 0,
        .watch_mode = 0,
        .watch_seconds = 0,
        .with_separator = 0,
    }};
    account_t account[1] = {{
        .username = {gbuff_alloca(MAX_VAR_LEN)},
        .password = {gbuff_alloca(MAX_VAR_LEN)},
    }};
    device_info_t devices[MAX_ONLINE_DEVICE_COUNT];
    size_t max_online_count = sizeof(devices) / sizeof(devices[0]);

    res = devices_get_config(config, argc, argv);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    // Fix env filepath
    if (config->env_filepath == NULL) {
        gbuff_t home_str[1] = {gbuff_alloca(MAX_PATH_LEN)};
        // fallback to default env
        res = get_defule_env_path(home_str);
        if (res != USTB_OK) {
            return EXIT_FAILURE;
        }
        config->env_filepath = home_str->data;
    }

    // Get username & password
    res = account_load_env(account, config->env_filepath);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    // Init HTTP
    http_t *http = alloca(HTTP_T_SIZE);
    res = http_init(http, DRCOM_HOST, DRCOM_PORT, IPV4_ONLY | HTTP_COOKIEJAR);
    if (res != 0) {
        return EXIT_FAILURE;
    }

    // Perform login
    res = devices_login(http, account);
    if (res != USTB_OK) {
        return EXIT_FAILURE;
    }

    do {
        // Fetch online devices
        int device_count =
            devices_check_online(devices, http, max_online_count);
        // Output
        devices_output(config, devices, device_count);

        if (config->watch_mode) {
            sleep(config->watch_seconds);
        }
    } while (config->watch_mode);

    return EXIT_SUCCESS;
}
