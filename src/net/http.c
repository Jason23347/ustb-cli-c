#include "conf.h"

#include "http.h"
#include "tcp.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF_SIZE 4096

typedef struct http {
    const char *domain;
    tcp_t conn;
    int ip_mode;
    uint16_t port;
} http_t;

http_t *
http_init(const char *domain, uint16_t port, int ip_mode) {
    http_t *http = malloc(sizeof(http_t));
    *http = (http_t){
        .domain = domain,
        .port = port,
        .ip_mode = ip_mode,
        .conn =
            {
                .fd = INVALID_SOCKET,
            },
    };

    return http;
}

void
http_free(http_t *http) {
    free(http);
}

int
http_connect(http_t *http) {
    int res = tcp_connect(&http->conn, http->domain, http->port, http->ip_mode);
    if (res != 0) {
        return -1;
    }

    return 0;
}

int
http_request(const http_t *http, const gstr_t *path) {
    /* Host请求头是增加幸福感的关键，不能删掉 */
    size_t req_len = path->len + 26 + strlen(http->domain);
    char req[req_len];
    snprintf(req, req_len,
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "\r\n",
             path->s, http->domain);

    size_t len = strlen(req);
    if (http_write(http, req, len) != len) {
        return -1;
    }

    return 0;
}

size_t
http_readline(const http_t *http, char *buf, size_t maxlen) {
    char c;
    char *p;
    int flag = 0;
    for (p = buf; http_read(http, &c, sizeof(c)) > 0; p++) {
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

ssize_t
http_read(const http_t *http, void *buf, size_t len) {
    return tcp_read(&http->conn, buf, len);
}

ssize_t
http_write(const http_t *http, void *buf, size_t len) {
    return tcp_write(&http->conn, buf, len);
}

void
http_close(const http_t *http) {
    tcp_close(&http->conn);
}

const char *
http_header(const http_t *http, char *buf, const char *header, size_t maxlen) {
    assert(header != NULL);

    size_t len;
    size_t used = 0;
    char *match = NULL;

    while (1) {
        len = http_readline(http, buf, maxlen - used);
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
http_skip_section(const http_t *http, char *buf, size_t maxlen) {
    size_t used = 0;
    while (1) {
        size_t len = http_readline(http, buf, maxlen - used);
        if (len == 0) {
            break;
        }
        used += len;
    }
}

char *
http_get(http_t *http, const gstr_t *path) {
    assert(path->s[0] == '/');

    char *body = NULL;

    char buf[MAX_BUF_SIZE];
    size_t len;

    int res = http_connect(http);
    if (res != 0) {
        return NULL;
    }

    res = http_request(http, path);
    if (res != 0) {
        return NULL;
    }

    /* 取content-length */
    const char *slen = http_header(http, buf, "content-length:", sizeof(buf));
    if (slen == NULL) {
        /**
         * 如果没设置content-length，再读一行直接返回。
         * 只需兼容 cippv6.ustb.edu.cn/get_ip.php
         */
        /* 这里有一行"3a"，是十六进制content-length */
        len = http_readline(http, buf, sizeof(buf));
        if (len <= 0) {
            http_close(http);
            return NULL;
        }
        sscanf(buf, "%lx", &len);
        body = calloc(len + 1, 1);
        if (body == NULL) {
            return NULL;
        }
        http_read(http, body, len);
    } else {
        /* 如果设置了content-length，再读一些字节 */
        len = atol(slen);
        /* 但是首先要把header过掉 */
        http_skip_section(http, buf, sizeof(buf));

        body = calloc(len + 1, 1);
        if (body == NULL) {
            return NULL;
        }
        http_read(http, body, len);
    }

    http_close(http);

    return body;
}
