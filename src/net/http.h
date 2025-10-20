#ifndef HTTP_H
#define HTTP_H

#include "gstr.h"
#include "socket.h"
#include "tcp.h"

#define MAX_BUFF_SIZE 4096

typedef struct {
    char buff[MAX_BUFF_SIZE];
    char domain[20]; // longest: "cippv6.ustb.edu.cn"
    tcp_t conn;
    uint16_t port;
    int ipv6_only;
} http_t;

int http_get(http_t *http, const gstr_t *path);

static inline int
http_get_root(http_t *http) {
    return http_get(http, &gstr_from_const("/"));
}

#endif /* HTTP_H */
