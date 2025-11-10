#ifndef HTTP_H
#define HTTP_H

#include "lib/gstr.h"
#include "tcp.h"

/* CookieJar flag */
#define HTTP_COOKIEJAR 4
#define MAX_BUF_SIZE   4096

typedef struct http http_t;
typedef struct cookiejar cookiejar_t;

http_t *http_init(const char *domain, uint16_t port, int http_mode);
/* Returns allocated body if success, else NULL */
char *http_request(http_t *http, const gstr_t *path, const gstr_t *data);
void http_free(http_t *http);

int http_connect(http_t *http);
int http_send_request(const http_t *http, const gstr_t *path,
                      const gstr_t *data);
ssize_t http_write(const http_t *http, void *buf, size_t len);
ssize_t http_read(const http_t *http, void *buf, size_t len);
void http_close(const http_t *http);

size_t http_readline(const http_t *http, char *buf, size_t maxlen);
size_t http_section(const http_t *http, char *buf, size_t maxlen);
void http_headers(const char **headers, char *raw_text, size_t count);
const char *http_find_header(const char **headers, const char *header,
                             size_t count);

static inline char *
http_get(http_t *http, const gstr_t *path) {
    return http_request(http, path, NULL);
}
static inline char *
http_get_root(http_t *http) {
    return http_get(http, &gstr_from_const("/"));
}

cookiejar_t *cookiejar_init(size_t maxlen);
int cookiejar_add(cookiejar_t *cookiejar, const char *key, const char *value);
int cookiejar_resolve(cookiejar_t *cookiejar, const char **headers,
                      size_t count);
const char *cookiejar_str(const cookiejar_t *cookiejar);
size_t cookiejar_length(const cookiejar_t *cookiejar);

#endif /* HTTP_H */
