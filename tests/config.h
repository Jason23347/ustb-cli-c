#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <stdlib.h>

#define UNIT_TESTING 1

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

/* Make VSCode happy */
#ifndef TEST_DIR
#define TEST_DIR ""
#endif

#if __WORDSIZE == 64
#define uint64_spec "%lu"
#else
#define uint64_spec "%llu"
#endif /* __WORDSIZE */

#define debug(...) ((void)0)

static inline int
cmd_parse(int argc, char **argv) {
    return 0;
}

static inline int
cmd_help(int argc, char **argv) {
    return 0;
}

struct cag_option;

/* Disable src/conf.h */
#ifndef CONF_H
#define CONF_H
#endif

#define LOGIN_HOST              TEST_DIR "/assets/login_normal.txt"
#define LOGIN_PORT              80
#define LOGIN_PATH              "/drcom/login"
#define CIPPV6_DOMAIN           TEST_DIR "/assets/cippv6_get_ip.txt"
#define CIPPV6_PORT             80
#define CIPPV6_PATH             "/get_ip.php"
#define SPEEDTEST_DOMAIN        "speed.ustb.edu.cn"
#define SPEEDTEST_PORT          80
#define SPEEDTEST_UPLOAD_PATH   "/backend/empty.php"
#define SPEEDTEST_DOWNLOAD_PATH "/backend/garbage.php"
#define USTB_ENV_FILENAME       ".ustb.env"
#define USTB_USERNAME_VAR       "USTB_USERNAME"
#define USTB_USERNAME_LEN       sizeof(USTB_USERNAME_VAR)
#define USTB_PASSWORD_VAR       "USTB_PASSWORD"
#define USTB_PASSWORD_LEN       sizeof(USTB_USERNAME_VAR)
#define INFO_REFRESH_INTERVAL   1
#define FREE_FLOW_GB            120
#define FEE_PER_1000GB          6120

#endif /* TEST_CONFIG_H */
