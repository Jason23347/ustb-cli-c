#include "conf.h"

#include "socket.h"
#include "tcp.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef WITH_SSL
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

static int
domain2addr(char *addr_str, const char *domain, size_t maxlen, int ip_mode) {
    int ret;

    int af;
    struct addrinfo *res;

    int use_ipv4 = ((ip_mode & IPV4_ONLY) != 0);
    int use_ipv6 = ((ip_mode & IPV6_ONLY) != 0);

    if (use_ipv4 && use_ipv6) {
        af = AF_UNSPEC;
    } else if (use_ipv4 && (!use_ipv6)) {
        af = AF_INET;
    } else if ((!use_ipv4) && use_ipv6) {
        af = AF_INET6;
    } else {
        /* Should output some warnings */
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

#ifdef WITH_SSL
int
ssl_init(tcp_t *tcp) {
    // 创建 SSL 上下文
    tcp->ssl_ctx = SSL_CTX_new(TLS_client_method());
    if (tcp->ssl_ctx == NULL) {
        return -1;
    }

    // 设置验证模式（使用 SSL_VERIFY_NONE 允许自签名证书）
    SSL_CTX_set_verify(tcp->ssl_ctx, SSL_VERIFY_NONE, NULL);

    // 加载默认的 CA 证书
    if (SSL_CTX_set_default_verify_paths(tcp->ssl_ctx) != 1) {
        print_log(DEBUG,
                  "SSL_CTX_set_default_verify_paths failed, continuing\n");
    }

    return 0;
}
#endif

/* Get a TCP connection */
int
tcp_connect(tcp_t *tcp, const char *domain, uint16_t port, int ip_mode) {
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
    int sock_fd = socket_connect(ip, port, is_ipv6);

    if (sock_fd == INVALID_SOCKET) {
        return -1;
    }
    tcp->fd = sock_fd;

    return 0;
}

ssize_t
tcp_read(const tcp_t *tcp, void *buffer, size_t size) {
#ifdef WITH_SSL
    if (tcp->ssl != NULL) {
        int n = SSL_read(tcp->ssl, buffer, size);
        if (n < 0) {
            int err = SSL_get_error(tcp->ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                // 需要重试，返回 0 表示暂时没有数据（调用者会重试）
                return 0;
            }
            // 其他错误返回 -1
            return -1;
        }
        // n >= 0，返回读取的字节数
        return n;
    }
#endif
    return read(tcp->fd, buffer, size);
}

ssize_t
tcp_write(const tcp_t *tcp, const void *buffer, size_t size) {
#ifdef WITH_SSL
    if (tcp->ssl != NULL) {
        int n = SSL_write(tcp->ssl, buffer, size);
        if (n < 0) {
            int err = SSL_get_error(tcp->ssl, n);
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                // 重试
                n = SSL_write(tcp->ssl, buffer, size);
            }
        }
        return n;
    }
#endif
    return write(tcp->fd, buffer, size);
}

#ifdef WITH_SSL

static void
tls_disconnect(tcp_t *tcp) {
    SSL_free(tcp->ssl);
    tcp->ssl = NULL;
}

#endif /* WITH_SSL */

void
tcp_close(tcp_t *tcp) {
#ifdef WITH_SSL
    if (tcp->ssl != NULL) {
        SSL_shutdown(tcp->ssl);
        tls_disconnect(tcp);
    }
#endif
    socket_close(tcp->fd);
}

#ifdef WITH_SSL

int
tls_upgrade(tcp_t *tcp, const char *sni_hostname) {
    if (tcp->ssl_ctx == NULL) {
        return -1;
    }

    // 创建 SSL 连接
    tcp->ssl = SSL_new(tcp->ssl_ctx);
    if (tcp->ssl == NULL) {
        print_log(ERROR, "SSL_new failed\n");
        return -1;
    }

    // 将 socket 文件描述符附加到 SSL
    if (SSL_set_fd(tcp->ssl, tcp->fd) != 1) {
        print_log(ERROR, "SSL_set_fd failed\n");
        tls_disconnect(tcp);
        return -1;
    }

    // 设置服务器名称（用于 SNI）- 由 HTTP 层传入域名
    if (sni_hostname != NULL &&
        SSL_set_tlsext_host_name(tcp->ssl, sni_hostname) != 1) {
        print_log(ERROR, "SSL_set_tlsext_host_name failed\n");
        tls_disconnect(tcp);
        return -1;
    }

    // 执行 SSL 握手（可能需要多次调用）
    int res = SSL_connect(tcp->ssl);
    while (res != 1) {
        int err = SSL_get_error(tcp->ssl, res);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            // 需要重试，使用 select 等待 socket 就绪
            fd_set read_fds, write_fds;
            struct timeval timeout;
            int select_res;

            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);
            timeout.tv_sec = 5;
            timeout.tv_usec = 0;

            if (err == SSL_ERROR_WANT_READ) {
                FD_SET(tcp->fd, &read_fds);
            } else {
                FD_SET(tcp->fd, &write_fds);
            }

            select_res = select(tcp->fd + 1,
                                err == SSL_ERROR_WANT_READ ? &read_fds : NULL,
                                err == SSL_ERROR_WANT_WRITE ? &write_fds : NULL,
                                NULL, &timeout);

            if (select_res <= 0) {
                print_log(ERROR, "SSL_connect: select timeout or error\n");
                tls_disconnect(tcp);
                return -1;
            }

            // 重试 SSL_connect
            res = SSL_connect(tcp->ssl);
        } else {
            // 其他错误
            unsigned long ssl_err = ERR_get_error();
            char err_buf[256];
            ERR_error_string_n(ssl_err, err_buf, sizeof(err_buf));
            print_log(ERROR, "SSL_connect failed: %s (error code: %d)\n",
                      err_buf, err);
            tls_disconnect(tcp);
            return -1;
        }
    }

    // 验证证书（如果启用了验证）
    if (SSL_get_verify_result(tcp->ssl) != X509_V_OK) {
        print_log(DEBUG,
                  "SSL certificate verification failed, continuing anyway\n");
    }

    return 0;
}

void
tls_cleanup(tcp_t *tcp) {
    if (tcp->ssl != NULL) {
        tls_disconnect(tcp);
    }
    if (tcp->ssl_ctx != NULL) {
        SSL_CTX_free(tcp->ssl_ctx);
        tcp->ssl_ctx = NULL;
    }
}

#endif /* WITH_SSL */
