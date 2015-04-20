#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

int use_tcpd = 0;

int SOCKET(int domain, int type, int protocol);
int BIND(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t SEND(int sockfd, const void *buf, size_t len, int flags);
ssize_t RECV(int sockfd, void *buf, size_t len, int flags);
int LISTEN(int sockfd, int backlog);
int CLOSE(int fd);
