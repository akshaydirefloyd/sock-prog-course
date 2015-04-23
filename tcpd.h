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

#define SOCK_TYPE_STREAM 0
#define SOCK_TYPE_DGRAM 1
#define MAX_ADDR_LIST 32
#define STDIN 0
#define MAX_FDS 32
#define MAX_BUF_SZ 512
#define PKT_TYPE_CONNECT_FTP2TCPD 1
#define PKT_TYPE_SEND_FTP2TCPD 2
#define PKT_TYPE_CONNECT_TCPD2TCPD 3
#define PKT_TYPE_SEND_TCPD2TCPD 4
#define PKT_TYPE_CONNECT_TCPD2FTP 5
#define PKT_TYPE_SEND_TCPD2FTP 6
static int fd_list[MAX_FDS];
static int fd_list_count = 0;

struct addrinfo **tcpd_addr_list;
int tcpd_addr_list_count = 0;

typedef struct trans_pkt {
    struct sockaddr addr;
    int type;
    int len;
    char payload[MAX_BUF_SZ];
    int seq_no;
} trans_pkt_t;

int setup_tcpd_addr(int sock_type, char *node_name,
		    char *port_num_str, struct addrinfo **ret_addr);
int get_tcpd_sock(struct addrinfo *addr);
int free_fds();
int free_tcpd_addresses();
