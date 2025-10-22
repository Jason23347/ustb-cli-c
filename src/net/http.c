#include "conf.h"

#include "http.h"
#include "tcp.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int
http_connect(http_t *http) {
    int res =
        tcp_connect(&http->conn, http->domain, http->port, http->ipv6_only);
    if (res != 0) {
        return -1;
    }

    return 0;
}

int
http_request(const http_t *http, const gstr_t *path) {
    /* Host请求头是增加幸福感的关键，不能删掉 */
    const char method[] = "GET";
    size_t req_len = path->len + 24 + sizeof(method) + sizeof(http->domain);
    char req[req_len];
    snprintf(req, req_len,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "\r\n",
             path->s, http->domain);

    if (tcp_write(&http->conn, req, strlen(req)) == -1) {
        return -1;
    }

    return 0;
}

size_t
http_readline(const http_t *http, size_t maxlen) {
    char *buf = http->buff;
    char c;
    char *p;
    int flag = 0;
    for (p = buf; tcp_read(&http->conn, &c, sizeof(c)) > 0; p++) {
        if (p - buf < maxlen) {
            if (c >= 'A' && c <= 'Z') {
                c += 0x20;
            }
            *p = c;
        } else {
            break;
        }

        if (c == '\r') {
            flag = 1;
        } else {
            if (flag == 1 && c == '\n') {
                // Got '\r\n'
                p--;
                break;
            } else {
                flag = 0;
            }
        }
    }

    *p = '\0';

    return p - buf;
}

size_t
http_read(http_t *http, size_t len) {
    return tcp_read(&http->conn, http->buff, len);
}

void
http_close(const http_t *http) {
    tcp_close(&http->conn);
}

const char *
http_header(const http_t *http, size_t maxlen, const char *header) {
    assert(header != NULL);

    size_t len;
    size_t used = 0;
    char *match = NULL;
    char *buf = http->buff;

    while (1) {
        len = http_readline(http, maxlen - used);
        if (len <= 0) { // empty line
            break;
        }

        match = strstr(buf, header);
        if (match != NULL) {
            match += strlen(header);
            break;
        }

        used += len;
    }

    return match;
}

void
http_skip_section(const http_t *http, size_t maxlen) {
    size_t used = 0;
    while (1) {
        size_t len = http_readline(http, maxlen - used);
        if (len == 0) {
            break;
        }
        used += len;
    }
}

int
http_get(http_t *http, const gstr_t *path) {
    assert(path->s[0] == '/');
    assert(http->buff == NULL);

    size_t len;

    int res = http_connect(http);
    if (res != 0) {
        return -1;
    }

    res = http_request(http, path);
    if (res != 0) {
        return -1;
    }

    /* 取content-length */
    char buf[MAX_BUF_SIZE];
    http->buff = buf;
    const char *slen = http_header(http, sizeof(buf), "content-length:");
    if (slen == NULL) {
        /**
         * 如果没设置content-length，再读一行直接返回。
         * 只需兼容 cippv6.ustb.edu.cn/get_ip.php
         */
        /* 不知为何这里有一行"3a"，何意味啊 */
        http_readline(http, sizeof(buf));
        len = http_readline(http, sizeof(buf));
        http->buff = malloc(len);
        if (http->buff == NULL) {
            return -1;
        }
        strncpy(http->buff, buf, len);
    } else {
        /* 如果设置了content-length，再读一些字节 */
        len = atol(slen);
        /* 但是首先要把header过掉 */
        http_skip_section(http, sizeof(buf));

        http->buff = malloc(len);
        if (http->buff == NULL) {
            return -1;
        }
        http_read(http, len);
    }

    http_close(http);

    return 0;
}

void
http_free(http_t *http) {
    free(http->buff);
#ifndef NDEBUG
    http->buff = NULL;
#endif
}
