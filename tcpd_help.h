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
#include <sys/time.h>
#include <fcntl.h>
#define MAX_BUF_SZ 512
#define PKT_TYPE_CONNECT_FTP2TCPD 1
#define PKT_TYPE_SEND_FTP2TCPD 2
#define PKT_TYPE_CONNECT_TCPD2TCPD 3
#define PKT_TYPE_SEND_TCPD2TCPD 4
#define PKT_TYPE_CONNECT_TCPD2FTP 5
#define PKT_TYPE_SEND_TCPD2FTP 6
#define TCPD_PORT 12345

typedef struct trans_pkt {
    struct sockaddr addr;
    int type;
    int len;
    char payload[MAX_BUF_SZ];
    int seq_no;
} trans_pkt_t;
ssize_t send_to_tcpd(int sockfd, void *buf, size_t len,
		     const struct sockaddr *addr, int flags);
ssize_t recv_from_tcpd(int sockfd, void *buf, size_t len, int flags);
//int send_connect_token_to_tcpd(int sockfd, char *buf, int len, int flags);
//int recv_accept_token_from_tcpd(int sockfd, char *buf, int len, int flags);
//
//ssize_t send_to_ftpc(const void *buf, size_t len, int flags);
//ssize_t recv_from_ftpc(const void *buf, size_t len, int flags);
//ssize_t send_to_ftps(const void *buf, size_t len, int flags);
//ssize_t recv_from_ftps(const void *buf, size_t len, int flags);
//
//ssize_t send_to_tcpds(const void *buf, size_t len, int flags);
//ssize_t recv_from_tcpds(const void *buf, size_t len, int flags);
//ssize_t send_to_tcpdc(const void *buf, size_t len, int flags);
//ssize_t recv_from_tcpdc(const void *buf, size_t len, int flags);
