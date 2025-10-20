#include "conf.h"

#include "http.h"
#include "tcp.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static const char *
http_header(const char *str, const char *header) {
    char *p, *end;

    if ((p = strstr(str, header)) == NULL) {
        return NULL;
    }
    if ((end = strstr(p, "\r\n")) == NULL) {
        return NULL;
    }
    *end = '\0';

    return p + strlen(header);
}

int
http_get(http_t *http, const gstr_t *path) {
    assert(path->s[0] == '/');

    int res =
        tcp_connect(&http->conn, http->domain, http->port, http->ipv6_only);
    if (res != 0) {
        return -1;
    }

    /* 极简版请求，因为随便发点就能从学校服务器得到返回 */
    /* 但是Host请求头是增加幸福感的关键，不能删掉 */
    const char method[] = "GET";
    size_t req_len = path->len + 16 + sizeof(method) + sizeof(http->domain);
    char req[req_len];
    snprintf(req, req_len,
             "GET %s\r\n"
             "Host: %s\r\n"
             "\r\n",
             path->s, http->domain);

    if (tcp_write(&http->conn, req, strlen(req)) == -1) {
        return -1;
    }

    /* 截至 \r\n\r\n 之前，包含所有 Headers */

    /**
     * 用于判断 Headers 结尾
     *
     * 0001 for ""
     * 0010 for "\r"
     * 0100 for "\r\n"
     * 1000 for "\r\n\r"
     * 0000 for "\r\n\r\n"
     */
    int8_t flag = 0x01;
    for (char *s = http->buff;; s++) {
        if (tcp_read(&http->conn, s, 1) <= 0) {
            // Connection gone.
            return -1;
        }

        if (*s == '\r' && (flag & 0x05)) {
            flag = flag << 1;
        } else if (*s == '\n' && (flag & 0x0a)) {
            flag = flag << 1;
        } else {
            /* 请求头全部转小写 */
            if (*s > 'A' && *s < 'Z')
                *s += 0x20;
            /* 重置 flag */
            flag = 0x01;
        }

        /* 当 flag 后四位为0时说明 Headers 部分结束 */
        if (!(flag & 0x0f)) {
            break;
        }
    }

    const char *slen = http_header(http->buff, "content-length:");
    if (slen != NULL) {
        size_t content_len = atoi(slen);
        tcp_read(&http->conn, http->buff, content_len);
    } else {
        // 此处兼容cippv6获取IPV6地址的接口，那个接口不返回请求头只有一行内容
    }

    tcp_close(&http->conn);

    return 0;
}
