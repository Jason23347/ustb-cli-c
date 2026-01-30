#ifndef TEST_CONFIG_H
#define TEST_CONFIG_H

#include <stdlib.h>

#define UNIT_TESTING 1

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

enum LOG_LEVEL {
    SILENT = 0,
};

#define log_set_file(...)  ((void)0)
#define log_set_level(...) ((void)0)
#define print_log(...)     ((void)0)

/* Disable src/conf.h */
#ifndef CONF_H
#define CONF_H
#endif

#define LOGIN_HOST       TEST_DIR "/assets/login_normal.txt"
#define LOGIN_PORT       80
#define LOGIN_PATH       "/drcom/login"
#define CIPPV6_DOMAIN    TEST_DIR "/assets/cippv6_get_ip.txt"
#define CIPPV6_PORT      80
#define CIPPV6_PATH      "/get_ip.php"
#define SPEEDTEST_DOMAIN "speed.ustb.edu.cn"
#ifdef USE_SSL
#define SPEEDTEST_PORT 443
#else
#define SPEEDTEST_PORT 80
#endif /* USE_SSL */
#define SPEEDTEST_UPLOAD_PATH   "/backend/empty.php"
#define SPEEDTEST_DOWNLOAD_PATH "/backend/garbage.php"
#define SPEEDTEST_PING_TESTS    10
#define DRCOM_HOST              "zifuwu.ustb.edu.cn"
#define DRCOM_PORT              443
#define DRCOM_FORM_PATH         "/Self/login"
#define DRCOM_RANDOMCODE_PATH   "/Self/login/randomCode"
#define DRCOM_LOGIN_PATH        "/Self/login/verify"
#define DRCOM_DEVICES_PATH      "/Self/dashboard/getOnlineList?order=asc"
#define USTB_ENV_FILENAME       ".ustb.env"
#define USTB_USERNAME_VAR       "USTB_USERNAME"
#define USTB_USERNAME_LEN       sizeof(USTB_USERNAME_VAR)
#define USTB_PASSWORD_VAR       "USTB_PASSWORD"
#define USTB_PASSWORD_LEN       sizeof(USTB_USERNAME_VAR)
#define INFO_REFRESH_INTERVAL   1

// https://zifuwu.ustb.edu.cn/Self/dashboard
// 赠122880MB，超出0.0006元/MB，4点登录，单向计费
#define FREE_FLOW_GB            120  // 122880MB
#define FEE_PER_1000GB          6144 // ￥0.0006/MB
#define MAX_ONLINE_DEVICE_COUNT 4    // 4点登录

#define PACKAGE_NAME    "MOCK"
#define VERSION_DISPLAY "dev"
#define CONFIGURE_TIME  "now"

#endif /* TEST_CONFIG_H */
