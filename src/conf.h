#ifndef CONF_H
#define CONF_H

#include "config.h"

#include <stdio.h>
#include <sys/types.h>

#if __WORDSIZE == 64
#define uint64_spec "%lu"
#else
#define uint64_spec "%llu"
#endif /* __WORDSIZE */

enum LOG_LEVEL {
    SILENT = 0,
    ERROR = 1,
    WARNING = 2,
    INFO = 3,
    DEBUG = 4,
};

void log_init(void) __attribute__((constructor));
void log_set_file(FILE *file);
void log_set_level(enum LOG_LEVEL level);
int print_log(enum LOG_LEVEL level, const char *fmt, ...);

#define LOGIN_HOST              "202.204.48.82"
#define LOGIN_PORT              80
#define LOGIN_PATH              "/drcom/login"
#define CIPPV6_DOMAIN           "cippv6.ustb.edu.cn"
#define CIPPV6_PORT             80
#define CIPPV6_PATH             "/get_ip.php"
#define SPEEDTEST_DOMAIN        "speed.ustb.edu.cn"
#define SPEEDTEST_PORT          443
#define SPEEDTEST_UPLOAD_PATH   "/backend/empty.php"
#define SPEEDTEST_DOWNLOAD_PATH "/backend/garbage.php"
#define DRCOM_HOST              "zifuwu.ustb.edu.cn"
#define DRCOM_PORT              443
#define DRCOM_FORM_PATH         "/Self/login/?302=LI"
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

#endif /* CONF_H */
