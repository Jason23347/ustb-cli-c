#define _GNU_SOURCE

#include "conf.h"

#include "http.h"
#include "tcp.h"

#include "lib/gbuff.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF_SIZE   4096
#define MAX_TRUNK_SIZE 64

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
http_send_request(const http_t *http, const gstr_t *path, const gstr_t *data,
                  const cookiejar_t *cookiejar) {
    size_t req_len = path->len + 33 + strlen(http->domain);
    size_t cookie_len = cookiejar_length(cookiejar);
    if ((cookiejar != NULL) && (cookie_len > 0)) {
        req_len += cookie_len + 10;
    }
    if (data != NULL) {
        req_len += data->len + 80;
    }
    gstr_t req[1] = {gstr_alloca(req_len)};
    /* Method & path */
    gstr_appendf(req, "%s %s HTTP/1.1\r\n", (data == NULL ? "GET" : "POST"),
                 path->data);
    /* Cookie */
    if ((cookiejar != NULL) && (cookie_len > 0)) {
        gstr_appendf(req, "Cookie: %s\r\n", cookiejar_str(cookiejar));
    }
    /* Form data */
    if (data != NULL) {
        gstr_appendf(req,
                     "Content-Type: application/x-www-form-urlencoded\r\n");
        gstr_appendf(req, "Content-Length: %d\r\n", data->len);
    }
    /* Host请求头是增加幸福感的关键，不能删掉 */
    if (http->port != 80) {
        gstr_appendf(req, "Host: %s:%u\r\n", http->domain, http->port);
    } else {
        gstr_appendf(req, "Host: %s\r\n", http->domain);
    }
    /* End of headers */
    gstr_appendf(req, "\r\n");

    /* Post data */
    if (data != NULL) {
        gstr_appendf(req, "%s\r\n\r\n", data->data);
    }

    size_t len = req->len;
    if (http_write(http, req->data, len) != len) {
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
            *p = c;
        } else {
            break;
        }

        if (c == '\r') {
            flag = 1;
        } else {
            if (flag == 1 && c == '\n') {
                // Got '\r\n'
                p++;
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
    ssize_t total = 0;
    char *p = buf;

    while (total < len) {
        ssize_t n = tcp_read(&http->conn, p + total, len - total);
        if (n < 0) {
            if (total == 0) {
                return n;
            } else {
                break;
            }
        }
        if (n == 0) {
            break;
        }
        total += n;
    }

    return total;
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
http_header(const http_t *http, char *buf, const char *header) {
    assert(header != NULL);

    char *match;
    char *p = buf;
    size_t len = strlen(header);

    while (1) {
        match = strcasestr(p, header);
        if (match != NULL) {
            p = match + len;
            if ((match <= buf) || (match[-1] == '\n')) {
                return p;
            }
        } else {
            return NULL;
        }
    }

    return NULL;
}

size_t
http_section(const http_t *http, char *buf, size_t maxlen) {
    size_t line_count;
    size_t used = 0;
    char *p = buf;

    for (line_count = 0; 1; line_count++) {
        size_t len = http_readline(http, p, maxlen - used);
        if (len == 2) {
            line_count--;
            break;
        }
        used += len;
        p += len;
    }

    return line_count;
}

void
http_headers(const char **headers, char *raw_text, size_t count) {
    size_t i = 0;
    const char crlf[] = "\r\n";
    char *p = raw_text;
    /* Skip 1st line "HTTP/1.1 200 ok" */
    do {
        p = strstr(p, crlf);
        if (p == NULL) {
            break;
        }
        *p = '\0';
        p += strlen(crlf);
        headers[i] = p;
        i++;
    } while (i < count);
}

const char *
http_find_header(const char **headers, const char *header, size_t count) {
    size_t hlen = strlen(header);

    assert(header[hlen - 1] == ':');

    for (size_t i = 0; i < count; i++) {
        const char *line = headers[i];
        if (strcasestr(line, header) != NULL) {
            return line;
        }
    }

    return NULL;
}

int
http_is_truncked(const char **headers, size_t count) {
    const char transfer_encoding[] = "transfer-encoding:";
    const char *header_value =
        http_find_header(headers, transfer_encoding, count);

    if (!header_value) {
        return 0;
    }
    header_value += strlen(transfer_encoding);
    /* trim spaces */
    while (header_value[0] == ' ') {
        header_value++;
    }
    if (strcmp(header_value, "chunked") == 0) {
        return 1;
    }

    return 0;
}

size_t
header_content_length(const char **headers, size_t count) {
    const char content_length[] = "content-length:";
    const char *slen = http_find_header(headers, content_length, count);

    if (slen == NULL) {
        return 0;
    }
    slen += strlen(content_length);
    return atol(slen);
}

static char *
http_body(http_t *http, const char **headers, size_t headers_count) {
    gbuff_t trunk[1], body[1];

    if (http_is_truncked(headers, headers_count)) {
        if (gbuff_init(trunk, MAX_TRUNK_SIZE) != 0) {
            return NULL;
        }
        if (gbuff_init(body, MAX_TRUNK_SIZE) != 0) {
            gbuff_free(trunk);
            return NULL;
        }

        while (1) {
            ssize_t n = http_readline(http, trunk->data, trunk->cap);
            if (n <= 0) {
                goto fail;
            }

            size_t chunk_len = strtoul(trunk->data, NULL, 16);
            if (chunk_len == 0) {
                // 跳过最后空行
                http_readline(http, trunk->data, trunk->cap);
                gbuff_free(trunk);
                gbuff_appendf(body, "", 1); // null-terminate
                return body->data;
            }

            /* Read into trunk */
            if (gbuff_ensure(trunk, chunk_len + 2) != 0) {
                goto fail;
            }
            if (http_read(http, trunk->data, chunk_len) != chunk_len) {
                goto fail;
            }

            /* Copy trunk into body */
            if (gbuff_ensure(body, body->len + chunk_len + 1) != 0) {
                goto fail;
            }

            if (gbuff_put(body, trunk->data, chunk_len) < 0) {
                goto fail;
            }

            // skip \r\n
            n = http_readline(http, trunk->data, trunk->cap);
            assert(n == 2);
        }

    fail:
        gbuff_free(trunk);
        gbuff_free(body);
        return NULL;
    } else {
        size_t len = header_content_length(headers, headers_count);
        if (gbuff_init(body, len + 1) != 0)
            return NULL;

        if (http_read(http, body->data, len) != len) {
            gbuff_free(body);
            return NULL;
        }

        body->len = len;
        body->data[len] = '\0';
        return body->data;
    }
}

char *
http_request(http_t *http, const gstr_t *path, const gstr_t *data,
             cookiejar_t *cookiejar) {
    assert(path->data[0] == '/');

    char *body = NULL;

    /* TCP connect */
    int res = http_connect(http);
    if (res != 0) {
        return NULL;
    }
    /* Send request */
    res = http_send_request(http, path, data, cookiejar);
    if (res != 0) {
        return NULL;
    }
    /* Headers */
    char buf[MAX_BUF_SIZE];
    size_t headers_count = http_section(http, buf, sizeof(buf));
    assert(headers_count < 16);
    const char **headers = alloca(headers_count * sizeof(char *));
    http_headers(headers, buf, headers_count);
    /* Cookies */
    if (cookiejar != NULL) {
        cookiejar_resolve(cookiejar, headers, headers_count);
    }
    /* Entire body */
    body = http_body(http, headers, headers_count);

    http_close(http);

    return body;
}
