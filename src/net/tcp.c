#include "tcp.h"

#include "socket.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static int
domain2addr(char *addr_str, const char *domain, size_t maxlen, int ip_mode) {
    int ret;

    int af;
    struct addrinfo *res;

    if (ip_mode == IPV4_ONLY) {
        af = AF_INET;
    } else if (ip_mode == IPV6_ONLY) {
        af = AF_INET6;
    } else {
        af = AF_UNSPEC;
    }

    struct addrinfo hints = {
        .ai_family = af,
        .ai_socktype = SOCK_STREAM,
    };

    ret = getaddrinfo(domain, NULL, &hints, &res);
    if (ret != 0) {
        return -1;
    }

    if (res->ai_family == AF_INET) {
        struct sockaddr_in *addr = (struct sockaddr_in *)res->ai_addr;
        snprintf(addr_str, maxlen, "%s", inet_ntoa(addr->sin_addr));
    } else if (res->ai_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)res->ai_addr;
        inet_ntop(AF_INET6, &addr6->sin6_addr, addr_str, maxlen);
    } else {
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);

    return 0;
}

static int
is_ipv4_address(const char *s) {
    struct in_addr addr4;
    return (inet_pton(AF_INET, s, &addr4) == 1);
}

static int
is_ipv6_address(const char *s) {
    struct in6_addr addr6;
    return (inet_pton(AF_INET6, s, &addr6) == 1);
}

/* Get a TCP connection */
int
tcp_connect(tcp_t *tcp, const char *domain, uint16_t port, int ip_mode) {
    int sock_fd = INVALID_SOCKET;
    const char *ip;
    char ip_buf[40];
    int is_ipv6;

    if (is_ipv4_address(domain)) {
        ip = domain;
        is_ipv6 = 0;
    } else if (is_ipv6_address(domain)) {
        ip = domain;
        is_ipv6 = 1;
    } else {
        domain2addr(ip_buf, domain, sizeof(ip_buf), ip_mode);
        ip = ip_buf;
        is_ipv6 = is_ipv6_address(ip);
    }

    socket_init();
    sock_fd = socket_connect(ip, port, is_ipv6);

    if (sock_fd == INVALID_SOCKET) {
        return -1;
    }
    tcp->fd = sock_fd;

    return 0;
}

ssize_t
tcp_read(const tcp_t *tcp, void *buffer, size_t size) {
    return read(tcp->fd, buffer, size);
}

ssize_t
tcp_write(const tcp_t *tcp, const void *buffer, size_t size) {
    return write(tcp->fd, buffer, size);
}

void
tcp_close(const tcp_t *tcp) {
    socket_close(tcp->fd);
}
