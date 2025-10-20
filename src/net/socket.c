#include "conf.h"

#include "socket.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define SOCKET_TIMEOUT 5

void
socket_init(void) {
    signal(SIGPIPE, SIG_IGN);
}

void
socket_close(SOCKET fd) {
    close(fd);
}

SOCKET
socket_connect(const char *ip, uint16_t port, int is_ipv6) {
    int res;
    SOCKET fd;
    socklen_t len;
    struct sockaddr_storage sa[1] = {0};

    if (is_ipv6) {
        struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)sa;
        sa6->sin6_family = AF_INET6;
        sa6->sin6_port = htons(port);
        if (inet_pton(AF_INET6, ip, &sa6->sin6_addr) != 1) {
            return INVALID_SOCKET;
        }
        fd = socket(AF_INET6, SOCK_STREAM, 0);
        len = sizeof(struct sockaddr_in6);
    } else {
        struct sockaddr_in *sa4 = (struct sockaddr_in *)sa;
        sa4->sin_family = AF_INET;
        sa4->sin_port = htons(port);
        sa4->sin_addr.s_addr = inet_addr(ip);
        fd = socket(AF_INET, SOCK_STREAM, 0);
        len = sizeof(struct sockaddr_in);
    }

    if (fd == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }

    /* non-blocking so can timeout */
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

    res = connect(fd, (const struct sockaddr *)&sa, len);
    if (res != -1) {
        fcntl(fd, F_SETFL, 0);
        return fd;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);
    /* set select() time out */
    struct timeval tv = {.tv_sec = SOCKET_TIMEOUT};
    res = select(fd + 1, NULL, &fdset, NULL, &tv);

    if (res == -1) {
        // TODO select error
        close(fd);
        return INVALID_SOCKET;
    } else if (res == 0) {
        // TODO timeout
        close(fd);
        return INVALID_SOCKET;
    }

    fcntl(fd, F_SETFL, 0);

    return fd;
}
