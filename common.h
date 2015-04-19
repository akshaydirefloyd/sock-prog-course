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
#define PORT_NUM_STR_MAX_LEN 256
#define REMOTE_HOST_STR_MAX_LEN 512
#define LOCAL_HOST_STR_MAX_LEN 512
#define LOCAL_FILE_NAME_MAX_LEN 512
#define REMOTE_FILE_NAME_MAX_LEN 512
#define SEND_BUF_SZ 512
#define RECV_BUF_SZ 512
#define SEND_BUF_SZ_TCPD 2000
#define RECV_BUF_SZ_TCPD 2000
#define TCPD_CRC_LENGTH 100
#define TCPD_MSS 1000
#define TCPDC_PORT 4567
#define TCPDS_PORT 4567

//Header exchange
typedef struct header_msg {
    char file_name[LOCAL_FILE_NAME_MAX_LEN];
    int file_size;
} header_msg_t;

//Encapsulation of resources associated with a socket connection
typedef struct sock_cxn {
    int send_port;
    int recv_port;
    char own_node_name[LOCAL_HOST_STR_MAX_LEN];
    char own_port_num_str[PORT_NUM_STR_MAX_LEN];
    char remote_node_name[REMOTE_HOST_STR_MAX_LEN];
    char remote_port_num_str[PORT_NUM_STR_MAX_LEN];
    char recv_buf[RECV_BUF_SZ_TCPD];
    char send_buf[SEND_BUF_SZ_TCPD];
    int cxn_sock;
    struct addrinfo *send_addr;
    struct addrinfo *recv_addr;
    struct addrinfo *own_free_addr;
    struct addrinfo *remote_free_addr;
    socklen_t send_addrlen;
    socklen_t recv_addrlen;
} sock_cxn_t;

//TCPD packet
typedef struct tcpd_packet {
    int seq_num;
    int buf_size; // should be <= TCPD_MSS
    char buf[TCPD_MSS];
    char checksum[TCPD_CRC_LENGTH];
} tcpd_packet_t;
