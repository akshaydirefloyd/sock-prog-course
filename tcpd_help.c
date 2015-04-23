#include "tcpd_help.h"

//int send_connect_token_to_tcpd(int sockfd, char *buf, int len, int flags)
//{
//    return 0;
//}
//
//int recv_accept_token_from_tcpd(int sockfd, char *buf, int len, int flags)
//{
//    return 0;
//}

ssize_t send_to_tcpd(int sockfd, void *buf, size_t len, const struct sockaddr *addr, int flags)
{   
    return sendto(sockfd, (const void *)buf, (size_t) len, (int) flags,
		  (const struct sockaddr *)addr, (socklen_t) sizeof(struct sockaddr));
}

ssize_t recv_from_tcpd(int sockfd, void *buf, size_t len, int flags)
{
    struct sockaddr src_addr;
    int addrlen = sizeof(src_addr);
    return recvfrom(sockfd, (void *)buf, (size_t) len, (int) flags,
		    (struct sockaddr *)&src_addr, (socklen_t *)&addrlen);
}
