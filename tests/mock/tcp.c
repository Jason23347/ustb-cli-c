#include "net/tcp.h"

#include "mem.h"

#include <fcntl.h>
#include <unistd.h>

int
tcp_connect(tcp_t *tcp, const char *domain, uint16_t port, int ip_mode) {
    tcp->fd = open(domain, O_RDONLY);
    if (tcp->fd < 0) {
        return -1;
    }

    return 0;
}

void
tcp_close(const tcp_t *tcp) {
    close(tcp->fd);
}

ssize_t
tcp_read(const tcp_t *tcp, void *buffer, size_t size) {
    return read(tcp->fd, buffer, size);
}

ssize_t
tcp_write(const tcp_t *tcp, const void *buffer, size_t size) {
    return size;
}

int
__wrap_tcp_connect(tcp_t *tcp, const char *domain, uint16_t port, int ip_mode) {
    return SHOULD_FAIL(-1, tcp_connect(tcp, domain, port, ip_mode));
}

ssize_t
__wrap_tcp_read(const tcp_t *tcp, void *buffer, size_t size) {
    return SHOULD_FAIL(-1, tcp_read(tcp, buffer, size));
}

ssize_t
__wrap_tcp_write(const tcp_t *tcp, const void *buffer, size_t size) {
    return SHOULD_FAIL(-1, tcp_write(tcp, buffer, size));
}
