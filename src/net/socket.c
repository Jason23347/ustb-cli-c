#include "conf.h"

#include "socket.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
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

static int
set_nonblocking(SOCKET s) {
    return fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) | O_NONBLOCK);
}

static int
set_blocking(SOCKET s) {
    return fcntl(s, F_SETFL, fcntl(s, F_GETFL, 0) & ~O_NONBLOCK);
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

    set_nonblocking(fd);
    res = connect(fd, (const struct sockaddr *)sa, len);

    if (res == -1 && errno != EINPROGRESS) {
        socket_close(fd);
        return INVALID_SOCKET;
    } else if (res == 0) {
        set_blocking(fd);
        return fd;
    }

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    struct timeval tv = {
        .tv_sec = SOCKET_TIMEOUT,
        .tv_usec = 0,
    };
    res = select(fd + 1, NULL, &fdset, NULL, &tv);

    if (res < 0) {
        socket_close(fd);
        return INVALID_SOCKET;
    } else if (res == 0) {
        print_log(DEBUG, "connect timed out\n");
        socket_close(fd);
        return INVALID_SOCKET;
    }

    int err = 0;
    socklen_t errlen = sizeof(err);
    int sockopt = getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &errlen);
    if ((sockopt != 0) || (err != 0)) {
        print_log(DEBUG, "connect failed after select: %d\n", err);
        socket_close(fd);
        return INVALID_SOCKET;
    }

    set_blocking(fd);

    return fd;
}
