#define _GNU_SOURCE

#include "conf.h"

#include "http.h"
#include "tcp.h"

#include "lib/gbuff.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUF_SIZE       4096
#define MAX_TRUNK_SIZE     64
#define MAX_COOKIEJAR_SIZE 128

typedef struct http_headers {
    const char **list;
    int count;
} http_headers_t;

typedef struct http {
    // Settings
    const char *domain;
    uint16_t port;
    int http_mode;

    tcp_t conn;
    cookiejar_t *cookiejar;

    // Results
    int status_code;
    http_headers_t headers[1];
    gstr_t body[1];
} http_t;

const size_t HTTP_T_SIZE = sizeof(struct http);

int
http_init(http_t *http, const char *domain, uint16_t port, int http_mode) {
    memset(http, 0, HTTP_T_SIZE);

    http->domain = domain;
    http->port = port;
    http->http_mode = http_mode;

    http->conn = (tcp_t){INVALID_SOCKET};

    if ((http_mode & HTTP_COOKIEJAR) != 0) {
        cookiejar_t *cookiejar = cookiejar_init(MAX_COOKIEJAR_SIZE);
        if (cookiejar == NULL) {
            goto fail;
        }
        http->cookiejar = cookiejar;
    } else {
        http->cookiejar = NULL;
    }

    if (gbuff_init(http->body, MAX_BUF_SIZE) != 0) {
        goto fail;
    }

    return 0;

fail:
    return -1;
}

void
http_free(http_t *http) {
    if (http->cookiejar) {
        cookiejar_free(http->cookiejar);
    }
    gbuff_free(http->body);
}

int
http_connect(http_t *http) {
    int res =
        tcp_connect(&http->conn, http->domain, http->port, http->http_mode);
    if (res != 0) {
        return -1;
    }

    return 0;
}

int
http_send_request(const http_t *http, const gstr_t *path, const gstr_t *data) {
    size_t req_len = path->len + 33 + strlen(http->domain);
    const cookiejar_t *cookiejar = http->cookiejar;
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

size_t
http_section(const http_t *http, char *buf, size_t maxlen) {
    size_t line_count;
    size_t used = 0;
    char *p = buf;

    for (line_count = 0; 1; line_count++) {
        size_t len = http_readline(http, p, maxlen - used);
        if (len <= 0) {
            break;
        } else if (len == 2) {
            line_count--;
            break;
        }
        used += len;
        p += len;
    }

    return line_count;
}

void
http_headers(const http_t *http, char *header_section) {
    const char crlf[] = "\r\n";
    char *p = header_section;
    const char **headers = http->headers->list;
    int count = http->headers->count;

    /* Skip 1st line "HTTP/1.1 200 ok" */
    // TODO get status from 1st line
    size_t i = 0;
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
http_find_header(const http_t *http, const char *header) {
    size_t hlen = strlen(header);

    const char **headers = http->headers->list;
    int count = http->headers->count;

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
http_is_chuncked(const http_t *http) {
    const char **headers = http->headers->list;

    const char transfer_encoding[] = "transfer-encoding:";
    const char *header_value = http_find_header(http, transfer_encoding);

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
header_content_length(const http_t *http) {
    const char content_length[] = "content-length:";
    const char *slen = http_find_header(http, content_length);

    if (slen == NULL) {
        return 0;
    }
    slen += strlen(content_length);
    return atol(slen);
}

static int
http_chuncked_body(http_t *http) {
    gbuff_t trunk[1];
    gbuff_t *body = http->body;

    if (gbuff_init(trunk, MAX_TRUNK_SIZE) != 0) {
        return -1;
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
            gbuff_appendf(body, ""); // null-terminate
            return 0;
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
    return -1;
}

static int
http_content_body(http_t *http) {
    gbuff_t *body = http->body;

    size_t len = header_content_length(http);
    if (gbuff_ensure(body, len + 1) != 0) {
        return -1;
    }

    ssize_t res = http_read(http, body->data, len);
    if (res != len) {
        return -1;
    }

    body->len = len;
    body->data[len] = '\0';
    return 0;
}

static int
http_body(http_t *http) {
    gbuff_clear(http->body);

    if (http_is_chuncked(http)) {
        return http_chuncked_body(http);
    } else {
        return http_content_body(http);
    }
}

const char *
http_request(http_t *http, const gstr_t *path, const gstr_t *data) {
    assert(path->data[0] == '/');

    /* TCP connect */
    int res = http_connect(http);
    if (res != 0) {
        return NULL;
    }
    /* Send request */
    res = http_send_request(http, path, data);
    if (res != 0) {
        return NULL;
    }
    /* Headers */
    char headers_section[MAX_BUF_SIZE];
    size_t headers_count =
        http_section(http, headers_section, sizeof(headers_section));
    if (headers_count == 0) {
        /* Unknown Error */
        return NULL;
    }
    http->headers->list = alloca(headers_count * sizeof(char *));
    http->headers->count = headers_count;

    http_headers(http, headers_section);
    /* Cookies */
    cookiejar_t *cookiejar = http->cookiejar;
    if (cookiejar != NULL) {
        cookiejar_resolve(cookiejar, http->headers->list, headers_count);
    }
    /* Entire body */
    res = http_body(http);
    if (res != 0) {
        return NULL;
    }
    /* Close socket */
    http_close(http);

    return http->body->data;
}
