#ifndef SOCKET_H
#define SOCKET_H

typedef int SOCKET;
static SOCKET INVALID_SOCKET = -1;

#include <stdint.h>

void socket_init(void);
void socket_end(void);
void socket_close(SOCKET fd);
SOCKET socket_connect(const char *ip, uint16_t port, int is_ipv6);

#endif /* SOCKET_H */
