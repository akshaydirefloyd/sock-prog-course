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

#define STDIN 0
#define SOCK_TYPE_STREAM 0
#define SOCK_TYPE_DGRAM 1
#define MAX_BUF_SZ 512
#define PKT_TYPE_CONNECT_FTP2TCPD 1
#define PKT_TYPE_SEND_FTP2TCPD 2
#define PKT_TYPE_CONNECT_TCPD2TCPD 3
#define PKT_TYPE_SEND_TCPD2TCPD 4
#define PKT_TYPE_CONNECT_TCPD2FTP 5
#define PKT_TYPE_SEND_TCPD2FTP 6
#define PKT_TYPE_ADDNODE_TCPD2TIMER 7
#define PKT_TYPE_DELNODE_TCPD2TIMER 8
#define PKT_TYPE_TIMEOUT_TIMER2TCPD 9
#define PKT_TYPE_LOOPBACK_TIMER2TIMER 10
#define DELTA_LENGTH 64 // Doesn't have to be greater than window size

struct addrinfo	*res = NULL;
struct addrinfo	*res_lb = NULL;

typedef struct trans_pkt {
    struct sockaddr addr;
    int type;
    int len;
    char payload[MAX_BUF_SZ];
    int seq_no;
} trans_pkt_t;

typedef struct timeout_node {
    int seq_no;
    int timeout_duration;
    int index;
    struct timeval start_time;
    struct timeout_node *next;
    struct timeout_node *prev;
} timeout_node_t;

timeout_node_t *delta_list = NULL;
timeout_node_t *delta_list_head_used = NULL;
timeout_node_t *delta_list_tail_used = NULL;
timeout_node_t *delta_list_head_free = NULL;
timeout_node_t *delta_list_tail_free = NULL;
int free_list_count = DELTA_LENGTH;
int used_list_count = 0;
