#define _GNU_SOURCE

#include "http.h"

#include <stdlib.h>

struct cookiejar {
    gstr_t str[1];
};

cookiejar_t *
cookiejar_init(size_t maxlen) {
    char *buf = malloc(maxlen);
    if (buf == NULL) {
        return NULL;
    }

    cookiejar_t *cookiejar = malloc(sizeof(cookiejar_t));
    if (cookiejar == NULL) {
        free(buf);
        return NULL;
    }

    cookiejar->str->data = buf;
    cookiejar->str->len = 0;
    cookiejar->str->cap = maxlen;
    cookiejar->str->data[0] = '\0';

    return cookiejar;
}

int
cookiejar_add(cookiejar_t *cookiejar, const char *key, const char *value) {
    if (cookiejar->str->len == 0) {
        return gstr_appendf(cookiejar->str, "%s=%s", key, value);
    } else {
        return gstr_appendf(cookiejar->str, "; %s=%s", key, value);
    }
}

static void
cookiejar_from_header(cookiejar_t *cookiejar, const char *header) {
    const char *p = header;

    /* Skip spaces */
    while (*p == ' ') {
        p++;
    }

    size_t maxlen = strlen(p);

    char *key = alloca(maxlen);
    char *value = alloca(maxlen);
    memset(key, 0, maxlen);
    memset(value, 0, maxlen);

    const char *eq = strchr(p, '=');
    if (eq == NULL) {
        // malformed cookie, no '=' found
        return;
    }
    // key 部分：从 p 到 '=' 前
    size_t key_len = eq - p;
    while (key_len > 0 && p[key_len - 1] == ' ') {
        // 去掉 key 末尾空格
        key_len--;
    }
    memcpy(key, p, key_len);
    key[key_len] = '\0';

    // value 部分：从 '=' 后开始
    const char *val_start = eq + 1;
    while (*val_start == ' ') {
        val_start++;
    }
    // 到分号或 CRLF 结束
    const char *val_end = strpbrk(val_start, ";\r\n");
    if (val_end == NULL) {
        val_end = val_start + strlen(val_start); // 无分号情况
    }
    // 去掉 value 末尾空格
    size_t val_len = val_end - val_start;
    while (val_len > 0 && (val_start[val_len - 1] == '=')) {
        val_len--;
    }
    memcpy(value, val_start, val_len);
    value[val_len] = '\0';

    cookiejar_add(cookiejar, key, value);
}

int
cookiejar_resolve(cookiejar_t *cookiejar, const char **headers, size_t count) {
    const char set_cookie[] = "set-cookie:";
    size_t slen = strlen(set_cookie);
    const char **head;
    size_t left = count;

    for (size_t i = 0; i < count; i++) {
        const char *line = headers[i];
        if (strcasestr(line, set_cookie) == NULL) {
            continue;
        }

        line += slen;

        cookiejar_from_header(cookiejar, line);
    }

    return 0;
}

const char *
cookiejar_str(const cookiejar_t *cookiejar) {
    if (cookiejar != NULL) {
        return cookiejar->str->data;
    } else {
        return NULL;
    }
}

size_t
cookiejar_length(const cookiejar_t *cookiejar) {
    if (cookiejar != NULL) {
        return cookiejar->str->len;
    } else {
        return 0;
    }
}
