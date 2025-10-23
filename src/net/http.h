#ifndef HTTP_H
#define HTTP_H

#include "gstr.h"
#include "tcp.h"

#define MAX_BUF_SIZE 4096

typedef struct http http_t;

http_t *http_init(const char *domain, uint16_t port, int ip_mode);
/* Returns allocated body if success, else NULL */
char *http_get(http_t *http, const gstr_t *path);
void http_free(http_t *http);

int http_connect(http_t *http);
/* 极简版请求，因为随便发点就能从学校服务器得到返回 */
int http_request(const http_t *http, const gstr_t *path);
const char *http_header(const http_t *http, char *buf, const char *header,
                        size_t maxlen);
size_t http_readline(const http_t *http, char *buf, size_t maxlen);
void http_skip_section(const http_t *http, char *buf, size_t maxlen);
size_t http_write(const http_t *http, void *buf, size_t len);
size_t http_read(http_t *http, void *buf, size_t len);
void http_close(const http_t *http);

static inline char *
http_get_root(http_t *http) {
    return http_get(http, &gstr_from_const("/"));
}

#endif /* HTTP_H */
