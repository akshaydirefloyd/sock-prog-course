#include "sock_wrapper_api.h"

int init_socks_api()
{
    use_tcpd = atoi(getenv("USE_TCPD"));
    if (0 == use_tcpd) {
	fprintf(stderr, "Not using using TCPD\n");
    }
    else if (1 == use_tcpd) {
	fprintf(stderr, "Using using TCPD\n");
    }
    else {
	fprintf(stderr, "USE_TCPD has to be 0 or 1\n");
	exit(-1);
    }
    return 0;
}

int SOCKET(int domain, int type, int protocol)
{
    if (0 == use_tcpd) {
	return socket(domain, type, protocol);
    }
}

int BIND(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (0 == use_tcpd) {
	return bind(sockfd, addr, addrlen);
    }
}

int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    if (0 == use_tcpd) {
	return accept(sockfd, addr, addrlen);
    }
}

int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (0 == use_tcpd) {
	return connect(sockfd, addr, addrlen);
    }
}

ssize_t SEND(int sockfd, const void *buf, size_t len, int flags)
{
    if (0 == use_tcpd) {
	return send(sockfd, buf, len, flags);
    }
}

ssize_t RECV(int sockfd, void *buf, size_t len, int flags)
{
    if (0 == use_tcpd) {
	return recv(sockfd, buf, len, flags);
    }
}

int LISTEN(int sockfd, int backlog)
{
    if (0 == use_tcpd) {
	return listen(sockfd, backlog);
    }
}

int CLOSE(int fd)
{
    if (0 == use_tcpd) {
	return close(fd);
    }
}
