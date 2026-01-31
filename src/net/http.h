#ifndef HTTP_H
#define HTTP_H

#include "lib/gbuff.h"
#include "tcp.h"

#define HTTP_COOKIEJAR 4  /* CookieJar flag */
#define HTTP_SSL       8  /* HTTPS flag */
#define HTTP_REDIRECT  16 /* Auto redirect flag */
#define MAX_BUF_SIZE   4096

typedef struct http http_t;
typedef struct cookiejar cookiejar_t;

extern const size_t HTTP_T_SIZE;

/* Initialize HTTP context; domain, port, and http_mode (flags) */
int http_init(http_t *http, const char *domain, uint16_t port, int http_mode);
/* Perform full request and return body pointer (in http's internal buffer), NULL on failure */
const char *http_request(http_t *http, const gbuff_t *path,
                         const gbuff_t *data);
/* Release internal resources; call http_close first if low-level API was used */
void http_free(http_t *http);

/* Establish TCP connection; performs SSL handshake if HTTPS is enabled */
int http_connect(http_t *http);
/* Send request line and headers; path must start with '/', use GET when data is NULL */
int http_send_request(const http_t *http, const gbuff_t *path,
                      const gbuff_t *data);
/* Write data to the connected socket */
ssize_t http_write(const http_t *http, void *buf, size_t len);
/* Read from the connected socket, blocks until buffer full or EOF */
ssize_t http_read(const http_t *http, void *buf, size_t len);
/* Close the underlying TCP connection */
void http_close(http_t *http);

/* Read a line until \r\n, store in buf with trailing \0, return length written */
size_t http_readline(const http_t *http, char *buf, size_t maxlen);
/* Read entire header section (until blank line), return number of header lines */
size_t http_section(const http_t *http, char *buf, size_t maxlen);

/* Return HTTP status code from last request, -1 on parse error */
int http_status_code(const http_t *http);

/* Disable automatic 302 redirect following */
void http_disable_redirect(http_t *http);
/* Enable automatic 302 redirect following (max 5 times) */
void http_enable_redirect(http_t *http);

/* Convenience wrapper for GET request */
static inline const char *
http_get(http_t *http, const gbuff_t *path) {
    return http_request(http, path, NULL);
}
/* Convenience wrapper to request root path "/" */
static inline const char *
http_get_root(http_t *http) {
    return http_get(http, &gbuff_from_const("/"));
}

/* Create cookie container; maxlen is internal string buffer capacity */
cookiejar_t *cookiejar_init(size_t maxlen);
/* Add a key=value cookie to cookiejar */
int cookiejar_add(cookiejar_t *cookiejar, const char *key, const char *value);
/* Parse Set-Cookie headers and merge into cookiejar */
int cookiejar_resolve(cookiejar_t *cookiejar, const char **headers,
                      size_t count);
/* Return string for Cookie header, format "key1=val1; key2=val2" */
const char *cookiejar_str(const cookiejar_t *cookiejar);
/* Return cookiejar string length, 0 if cookiejar is NULL */
size_t cookiejar_length(const cookiejar_t *cookiejar);
/* Release cookiejar and its internal resources */
void cookiejar_free(cookiejar_t *cookiejar);

#endif /* HTTP_H */
