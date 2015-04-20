#include "sock_wrapper_api.h"

int SOCKET(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}

int BIND(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}

int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    return accept(sockfd, addr, addrlen);
}

int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return connect(sockfd, addr, addrlen);
}

ssize_t SEND(int sockfd, const void *buf, size_t len, int flags)
{
    return send(sockfd, buf, len, flags);
}

ssize_t RECV(int sockfd, void *buf, size_t len, int flags)
{
    return recv(sockfd, buf, len, flags);
}

int LISTEN(int sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

int CLOSE(int fd)
{
    return close(fd);
}

