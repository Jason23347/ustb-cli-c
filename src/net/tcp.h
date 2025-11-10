#ifndef TCP_H
#define TCP_H

#include "socket.h"

#include <stdint.h>
#include <sys/types.h>

#define IPV4_ONLY 1
#define IPV6_ONLY 2
#define IPV4_IPV6 (IPV4_ONLY | IPV6_ONLY)

typedef struct {
    SOCKET fd;
} tcp_t;

int tcp_connect(tcp_t *tcp, const char *domain, uint16_t port, int ip_mode);
void tcp_close(const tcp_t *tcp);

ssize_t tcp_read(const tcp_t *tcp, void *buffer, size_t size);
ssize_t tcp_write(const tcp_t *tcp, const void *buffer, size_t size);

#endif /* TCP_H */
