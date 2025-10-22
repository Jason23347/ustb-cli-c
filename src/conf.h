#ifndef CONF_H
#define CONF_H

#include "config.h"

#include <sys/types.h>

#if __WORDSIZE == 64
#define uint64_spec "%lu"
#else
#define uint64_spec "%llu"
#endif /* __WORDSIZE */

#ifndef NDEBUG

#define debug(fmt, ...) debugf("%s: " fmt, __FUNCTION__, ##__VA_ARGS__)

int debugf(const char *fmt, ...);

#else
#define debug(...)
#endif /* NDEBUG */

#define LOGIN_HOST              "202.204.48.82"
#define LOGIN_PORT              80
#define LOGIN_PATH              "/drcom/login"
#define CIPPV6_DOMAIN           "cippv6.ustb.edu.cn"
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

#endif /* CONF_H */
