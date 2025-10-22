#ifndef HTTP_H
#define HTTP_H

#include "gstr.h"
#include "tcp.h"

#define MAX_BUF_SIZE 4096

typedef struct {
    char *buff;
    char domain[20]; // longest: "cippv6.ustb.edu.cn"
    tcp_t conn;
    uint16_t port;
    int ipv6_only;
} http_t;

int http_connect(http_t *http);
/* 极简版请求，因为随便发点就能从学校服务器得到返回 */
int http_request(const http_t *http, const gstr_t *path);
const char *http_header(const http_t *http, size_t maxlen, const char *header);
size_t http_readline(const http_t *http, size_t maxlen);
void http_skip_section(const http_t *http, size_t maxlen);
size_t http_read(http_t *http, size_t len);
void http_close(const http_t *http);

int http_get(http_t *http, const gstr_t *path);
void http_free(http_t *http);

static inline int
http_get_root(http_t *http) {
    return http_get(http, &gstr_from_const("/"));
}

#endif /* HTTP_H */
