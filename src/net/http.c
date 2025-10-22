#include "conf.h"

#include "http.h"
#include "tcp.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

/* 极简版请求，因为随便发点就能从学校服务器得到返回 */
static int
http_request(const http_t *http, const gstr_t *path) {
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

    return 0;
}

size_t
http_readline(const tcp_t *tcp, char *line, size_t maxlen) {
    char c;
    char *p;
    int flag = 0;
    for (p = line; tcp_read(tcp, &c, sizeof(c)) > 0; p++) {
        if (p - line < maxlen) {
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

    return p - line;
}

int
http_get(http_t *http, const gstr_t *path) {
    assert(path->s[0] == '/');
    assert(http->buff == NULL);

    const char *slen;
    size_t len;

    int res =
        tcp_connect(&http->conn, http->domain, http->port, http->ipv6_only);
    if (res != 0) {
        return -1;
    }

    res = http_request(http, path);
    if (res != 0) {
        return -1;
    }

    /* 从header段取content-length */
    char buff[MAX_BUFF_SIZE];
    char *line = buff;

    while (1) {
        size_t used = line - buff;
        len = http_readline(&http->conn, line, sizeof(buff) - used);
        if (len <= 0) { // empty line
            break;
        }

        const char content_length[] = "content-length";
        char *match = strstr(line, content_length);
        if (match != NULL) {
            slen = match + sizeof(content_length);
            break;
        }

        line += len;
    }

    if (slen == NULL) {
        /**
         * 如果没设置content-length，直接返回当前body字符串。
         * 虽然有的\r被替换成0，但是目前只需兼容cippv6，不影响。
         */
        len = line - buff;
        http->buff = malloc(len);
        if (http->buff == NULL) {
            return -1;
        }
        strncpy(http->buff, buff, len);
    } else {
        /* 如果设置了content-length，再读一些字节 */
        len = atoi(slen);
        http->buff = malloc(len);
        if (http->buff == NULL) {
            return -1;
        }
        tcp_read(&http->conn, http->buff, len);
    }

    tcp_close(&http->conn);

    return 0;
}

void
http_free(http_t *http) {
    free(http->buff);
#ifndef NDEBUG
    http->buff = NULL;
#endif
}
