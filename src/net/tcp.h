#ifndef TCP_H
#define TCP_H

#include "socket.h"

#include <stddef.h>
#include <stdint.h>

typedef struct {
    SOCKET fd;
} tcp_t;

int tcp_connect(tcp_t *tcp, const char *domain, uint16_t port, int ipv6_only);
void tcp_close(tcp_t *tcp);

size_t tcp_read(tcp_t *tcp, void *buffer, size_t size);
size_t tcp_write(tcp_t *tcp, const void *buffer, size_t size);

#endif /* TCP_H */
