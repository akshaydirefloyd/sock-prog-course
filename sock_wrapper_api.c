#include "sock_wrapper_api.h"
#include "tcpd_help.h"
#include "common.h"

int use_tcpd = 0;
static int socket_bound = 0;
static int socket_listening = 0;

struct sockaddr *server_addr = NULL;
struct sockaddr *client_addr = NULL;
struct sockaddr *tcpd_addr = NULL;

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
    else {
	int rval = -1;
	rval = socket(domain, SOCK_DGRAM, 0);
	if(-1 == rval) {
	    fprintf(stderr, "Error in socket:%s\n", strerror(errno));
	    exit(-1);
	}
	fprintf(stderr, "Socket(%d) created\n", rval);
	return rval;
    }
}

int BIND(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (0 == use_tcpd) {
	return bind(sockfd, addr, addrlen);
    }
    else {
	//Make tcpd listen be capable of listening at the specified sock
	//which is identical to what needs to be done even if not
	//using tcpd
	socket_bound = 1;
	fprintf(stderr, "Socket(%d) bound\n", sockfd);
	return bind(sockfd, addr, addrlen);
    }
    fprintf(stderr, "Socket bound\n");
}

int ACCEPT(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    if (0 == use_tcpd) {
	return accept(sockfd, addr, addrlen);
    }
    else {
	// wait for the first byte and also return the right address
	// and length
	trans_pkt_t accept_pkt;
	int len = sizeof(trans_pkt_t);
	int rlen = -1;
	rlen = recv_from_tcpd(sockfd, (char *)&accept_pkt, len, 0);
	fprintf(stderr, "Expecting pkt type %d, recved pkt type %d\n",
		PKT_TYPE_CONNECT_TCPD2FTP, accept_pkt.type);
	if (PKT_TYPE_CONNECT_TCPD2FTP == accept_pkt.type) {
	    fprintf(stderr, "Connect request received from client\n");
	    memcpy(addr, &(accept_pkt.payload), sizeof(struct sockaddr));
	    *addrlen = (socklen_t) accept_pkt.len;
	    fprintf(stderr, "ACCEPT() Received a packet of size = %d [sizeof(struct of sa) = %d]",
		    (int) *addrlen, (int) sizeof(struct sockaddr));
	    return sockfd;
	}
	//rlen = recv_accept_token_from_tcpd((char *)&accept_pkt, len, 0);
	return 0;
    }
}

int CONNECT(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    if (0 == use_tcpd) {
	return connect(sockfd, addr, addrlen);
    }
    else {
	//should be a no-op but will send a token to give ftps the
	//address that it seeks

	trans_pkt_t connect_pkt;
	char tcpd_hostname[256];
	char tcpd_portnum[256];
	struct addrinfo *tcpd_addrinfo;
	
	memset(&connect_pkt, 0, sizeof(trans_pkt_t));
	memcpy(&(connect_pkt.addr), addr, sizeof(struct sockaddr));
	connect_pkt.type = PKT_TYPE_CONNECT_FTP2TCPD;
	connect_pkt.len = sizeof(struct sockaddr);

	// This server address will be used for all future references in send
	server_addr = (struct sockaddr *)addr;

	// setup connection to tcpd
	if(-1 == gethostname(tcpd_hostname, 256)) {
	    fprintf(stderr, "Error in gethostname:%s\n", strerror(errno));
	    exit(-1);
	}

	sprintf(tcpd_portnum, "%d", TCPD_PORT);

	setup_addr(SOCK_TYPE_DGRAM, tcpd_hostname, tcpd_portnum, &tcpd_addrinfo);
	tcpd_addr = tcpd_addrinfo->ai_addr;
	
	//return send_connect_token_to_tcpd((char *)&connect_pkt, sizeof(trans_pkt_t), 0);
	if (NULL == tcpd_addr) {
	    fprintf(stderr, "Trying to send without setting up tcpd address\n");
	    exit(-1);
	}

	fprintf(stderr, "Attempting to connect\n");
	return send_to_tcpd(sockfd, (char *)&connect_pkt, sizeof(trans_pkt_t), tcpd_addr, 0);
    }
}

ssize_t SEND(int sockfd, const void *buf, size_t len, int flags)
{
    sleep(1);
    if (0 == use_tcpd) {
	return send(sockfd, buf, len, flags);
    }
    else {
	// send to tcpd
	int rlen = -1;
	if(NULL == tcpd_addr) {
	    fprintf(stderr, "Trying to send without setting up tcpd address\n");
	    exit(-1);
	}

	trans_pkt_t send_pkt;
	
	memset(&send_pkt, 0, sizeof(trans_pkt_t));
	if (NULL == server_addr) {
	    fprintf(stderr, "Server addr not set - unexpected\n");
	    exit(-1);
	}
	memcpy(&(send_pkt.addr), server_addr, sizeof(struct sockaddr));
	send_pkt.type = PKT_TYPE_SEND_FTP2TCPD;
	send_pkt.len = len;
	memcpy((void *)((char *)send_pkt.payload), (void *)((char *)buf), len);
	if (len == 4) {
	    //fprintf(stderr, "SEND(4) %d\n", (int) *(send_pkt.payload));
	    send_pkt.seq_no = 4;
	}
	if (len == 128) {
	    send_pkt.seq_no = 128;
	    char temp_name[128];
	    memcpy(temp_name, buf, len);
	    //fprintf(stderr, "buf = %s\n", temp_name);
	    memcpy(temp_name, send_pkt.payload, len);
	    //fprintf(stderr, "payload = %s\n", temp_name);
	}
	// tcpd always packs in more so need to subtract overhead
	rlen = send_to_tcpd(sockfd, (char *)&send_pkt, sizeof(trans_pkt_t), tcpd_addr, flags);
	if (rlen == sizeof(trans_pkt_t)) {
	    return len;
	}
	else {
	    // Not all the data requested was sent out.
	    // Need to handle this case
	    fprintf(stderr, "SEND() unable to send out all data\n");
	    exit(-1);
	}
    }
}

ssize_t RECV(int sockfd, void *buf, size_t len, int flags)
{
    if (0 == use_tcpd) {
	return recv(sockfd, buf, len, flags);
    }
    else {
	// recv from tcpd
	if (1 == socket_listening) {
	    int rlen = -1;
	    trans_pkt_t recv_pkt;
	    rlen = recv_from_tcpd(sockfd, &recv_pkt, sizeof(trans_pkt_t), flags);
	    
	    if (PKT_TYPE_SEND_TCPD2FTP == recv_pkt.type) {
		fprintf(stderr, "Send request received from client\n");
	    }
	    else {
		fprintf(stderr, "RECV() Pkt type unexpected\n");
	    }
	    
	    if (len == recv_pkt.len) {
		fprintf(stderr, "Receive request and pkt payload length match\n");
	    }
	    else {
		fprintf(stderr, "Receive request (%d) and pkt payload length (%d) mismatch\n",
			(int) len, (int) recv_pkt.len);
		exit(-1);
	    }
	    
	    memcpy(buf, &(recv_pkt.payload), recv_pkt.len);
	    return recv_pkt.len;
	}
	else {
	    fprintf(stderr, "socket not listening but recv attempted\n");
	    exit(-1);
	}
    }
}

int LISTEN(int sockfd, int backlog)
{
    if (0 == use_tcpd) {
	return listen(sockfd, backlog);
    }
    else {
	//Needs to be called after bind after which tcpd starts
	//listening for connections?
	if (1 == socket_bound) {
	    socket_listening = 1;
	    // setup connection to tcpd
	    char tcpd_hostname[512];
	    char tcpd_portnum[256];
	    struct addrinfo *tcpd_addrinfo;
	    if(-1 == gethostname(tcpd_hostname, 512)) {
		fprintf(stderr, "Error in gethostname:%s\n", strerror(errno));
		exit(-1);
	    }

	    sprintf(tcpd_portnum, "%d", TCPD_PORT);

	    setup_addr(SOCK_TYPE_DGRAM, tcpd_hostname, tcpd_portnum, &tcpd_addrinfo);
	    tcpd_addr = tcpd_addrinfo->ai_addr;
	}
	else {
	    fprintf(stderr, "Program trying to listen without binding\n");
	    exit(-1);
	}
    }
    return 0;
}

int CLOSE(int fd)
{
    if (0 == use_tcpd) {
	return close(fd);
    }
    else {
	//close the ud socket which connects ftp* to tcpd
	//Again this is identical to the case when tcpd isn't used
	return close(fd);
    }
}
