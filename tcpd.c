#include "tcpd.h"


// Free all initiated addrinfo structures
int free_tcpd_addresses()
{
    while (0 != tcpd_addr_list_count) {
	tcpd_addr_list_count--;
	freeaddrinfo(tcpd_addr_list[tcpd_addr_list_count]);
    }
    free(tcpd_addr_list);
    return 0;
}

int setup_tcpd_addr(int sock_type, char *node_name, char *port_num_str, struct addrinfo **ret_addr)
{
    struct addrinfo *res, *res_ptr_iter, hints;
    char ip_addr_str[128];
    struct sockaddr_in *ip_sock_addr;
    int rval = -1;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   //Need IPV4
    hints.ai_flags = AI_PASSIVE; //Free to choose any of the local interfaces
    if (sock_type == SOCK_TYPE_STREAM) {
	hints.ai_socktype = SOCK_STREAM; //Need TCP sockets
    }
    else {
	hints.ai_socktype = SOCK_DGRAM; //Need UDP sockets
    }

    rval = getaddrinfo(node_name, port_num_str, &hints, &res);
    if (0 != rval) {
	fprintf(stderr, "Error in getaddrinfo: %s, rval = %d port_num = %s\n",
		strerror(errno), rval, port_num_str);
	exit(-1);
    }

    //Go through valid address list and pick out first match
    //The following iteration isn't really necessary
    for (res_ptr_iter = res; res_ptr_iter != NULL; 
	 res_ptr_iter = res_ptr_iter->ai_next) {
	if (sock_type == SOCK_TYPE_DGRAM) {
	    if ((res_ptr_iter->ai_family == AF_INET) && 
		(res_ptr_iter->ai_socktype == SOCK_DGRAM)) {
		ip_sock_addr = (struct sockaddr_in *) res_ptr_iter->ai_addr;
		if (NULL == inet_ntop(res_ptr_iter->ai_family, &(ip_sock_addr->sin_addr), 
				      ip_addr_str, sizeof(ip_addr_str))) {
		    fprintf(stderr, "Error in inet_ntop : %s\n", strerror(errno));
		}
		fprintf(stderr, "Obtained address for %s = %s\n", node_name, ip_addr_str);
		*ret_addr = res_ptr_iter;
		break;
	    }
	}
	else {
	    if ((res_ptr_iter->ai_family == AF_INET) && 
		(res_ptr_iter->ai_socktype == SOCK_STREAM)) {
		ip_sock_addr = (struct sockaddr_in *) res_ptr_iter->ai_addr;
		if (NULL == inet_ntop(res_ptr_iter->ai_family, &(ip_sock_addr->sin_addr), 
				      ip_addr_str, sizeof(ip_addr_str))) {
		    fprintf(stderr, "Error in inet_ntop : %s\n", strerror(errno));
		}
		fprintf(stderr, "Obtained address for %s = %s\n", node_name, ip_addr_str);
		*ret_addr = res_ptr_iter;
		break;
	    }
	}
    }

    //Keep track of results iterator for freeing later
    if (0 == tcpd_addr_list_count) {
	tcpd_addr_list = (struct addrinfo **) malloc(sizeof(struct addrinfo *) * MAX_ADDR_LIST);
    }
    
    tcpd_addr_list[tcpd_addr_list_count] = res;
    tcpd_addr_list_count++;
    
    return 0;
}

int get_addr(int sock_type, char *node_name, char *port_num_str, struct addrinfo **ret_addr)
{
    struct addrinfo *res, *res_ptr_iter, hints;
    char ip_addr_str[128];
    struct sockaddr_in *ip_sock_addr;
    int rval = -1;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   //Need IPV4
    hints.ai_flags = AI_PASSIVE; //Free to choose any of the local interfaces
    if (sock_type == SOCK_TYPE_STREAM) {
	hints.ai_socktype = SOCK_STREAM; //Need TCP sockets
    }
    else {
	hints.ai_socktype = SOCK_DGRAM; //Need UDP sockets
    }

    rval = getaddrinfo(node_name, port_num_str, &hints, &res);
    if (0 != rval) {
	fprintf(stderr, "Error in getaddrinfo: %s, rval = %d port_num = %s\n",
		strerror(errno), rval, port_num_str);
	exit(-1);
    }

    //Go through valid address list and pick out first match
    //The following iteration isn't really necessary
    for (res_ptr_iter = res; res_ptr_iter != NULL; 
	 res_ptr_iter = res_ptr_iter->ai_next) {
	if (sock_type == SOCK_TYPE_DGRAM) {
	    if ((res_ptr_iter->ai_family == AF_INET) && 
		(res_ptr_iter->ai_socktype == SOCK_DGRAM)) {
		ip_sock_addr = (struct sockaddr_in *) res_ptr_iter->ai_addr;
		if (NULL == inet_ntop(res_ptr_iter->ai_family, &(ip_sock_addr->sin_addr), 
				      ip_addr_str, sizeof(ip_addr_str))) {
		    fprintf(stderr, "Error in inet_ntop : %s\n", strerror(errno));
		}
		fprintf(stderr, "Obtained address for %s = %s\n", node_name, ip_addr_str);
		*ret_addr = res_ptr_iter;
		break;
	    }
	}
	else {
	    if ((res_ptr_iter->ai_family == AF_INET) && 
		(res_ptr_iter->ai_socktype == SOCK_STREAM)) {
		ip_sock_addr = (struct sockaddr_in *) res_ptr_iter->ai_addr;
		if (NULL == inet_ntop(res_ptr_iter->ai_family, &(ip_sock_addr->sin_addr), 
				      ip_addr_str, sizeof(ip_addr_str))) {
		    fprintf(stderr, "Error in inet_ntop : %s\n", strerror(errno));
		}
		fprintf(stderr, "Obtained address for %s = %s\n", node_name, ip_addr_str);
		*ret_addr = res_ptr_iter;
		break;
	    }
	}
    }

    //Keep track of results iterator for freeing later
    if (0 == tcpd_addr_list_count) {
	tcpd_addr_list = (struct addrinfo **) malloc(sizeof(struct addrinfo *) * MAX_ADDR_LIST);
    }
    
    tcpd_addr_list[tcpd_addr_list_count] = res;
    tcpd_addr_list_count++;
    
    return 0;
}

int get_tcpd_sock(struct addrinfo *addr)
{
    int tcpd_socket = -1;
    
    tcpd_socket = socket(addr->ai_family, addr->ai_socktype, 
			      addr->ai_protocol);
    if (-1 == tcpd_socket) {
	fprintf(stderr, "Error in socket creation:%s\n", strerror(errno));
	exit(-1);
    }

    if (-1 == bind(tcpd_socket, addr->ai_addr, 
		   addr->ai_addrlen)) {
	fprintf(stderr, "Error in bind:%s\n", strerror(errno));
	exit(-1);
    }
    
    fd_list[fd_list_count++] = tcpd_socket;
    
    return tcpd_socket;
}

int free_fds()
{
    while (0 != fd_list_count) {
	fd_list_count--;
	close(fd_list[fd_list_count]);
    }
    return 0;
}

ssize_t send_message(int sock, char *dst_name, char *dst_port,
		     const void *buf, size_t len, int flags)
{
    struct addrinfo *dst_addr;
    int size_addr = sizeof(struct sockaddr_storage);
    
    get_addr(SOCK_TYPE_DGRAM, dst_name, dst_port, &dst_addr);
    return sendto(sock, buf, len, 0, dst_addr->ai_addr, (socklen_t) size_addr);
}

ssize_t forward_message(int sock, struct sockaddr *addr, const void *buf,
			size_t len, int flags)
{
    int size_addr = sizeof(struct sockaddr_storage);

    return sendto(sock, buf, len, 0, addr, (socklen_t) size_addr);
}


ssize_t recv_message(int sock, void *buf, size_t len,
		     int flags, struct sockaddr *src_addr)
{
    int size_addr = sizeof(struct sockaddr);
    int rval;
    
    rval = recvfrom(sock, (void *)buf, len, 0, src_addr, (socklen_t *)&size_addr);
    if (rval == -1) {
	fprintf(stderr, "recvfrom:%s\n", strerror(errno));
    }
    return rval;
}

int main(int argc, char **argv)
{
    struct timeval tv;
    int sock = -1;
    fd_set readfds[2];
    char msg[1024];
    struct addrinfo *tcpd_addr;
    char *port_num_str;

    // Check if arguments passed are correct
    if (argc < 2) {
	fprintf(stderr, "Usage: ./tcpd port-number");
	exit(-1);
    }
    else {
	port_num_str = argv[1];
    }

    setup_tcpd_addr(SOCK_TYPE_DGRAM, NULL, port_num_str, &tcpd_addr);
    sock = get_tcpd_sock(tcpd_addr);
    
    tv.tv_sec = 40;
    tv.tv_usec = 0;

    while(1) {
	FD_ZERO(readfds);
	FD_SET(STDIN, readfds);
	FD_SET(sock, readfds);
	if (sock > STDIN) {
	    select(sock + 1, readfds, NULL, NULL, &tv);
	}
	else {
	    select(STDIN + 1, readfds, NULL, NULL, &tv);
	}

	if (FD_ISSET(STDIN, readfds)) {
	    fprintf(stderr, "key pressed\n");
	    scanf("%s", msg);
	    fprintf(stderr, "%s\n", msg);
	    if (msg[0] == 's') {
		char temp_hostname[256];
		char temp_portnum[256];
		char temp_buf[256];
		int rlen;
		
		fprintf(stderr, "Enter hostname:");
		scanf("%s", temp_hostname);
		fprintf(stderr, "Enter portnum:");
		scanf("%s", temp_portnum);
		do {
		    fprintf(stderr, "Enter msg:");
		    scanf("%s", temp_buf);

		    if (0 == strcmp("X", temp_buf)) {
			fprintf(stderr, "Closing up..\n");
			break;
		    }
		    rlen = send_message(sock, temp_hostname, temp_portnum,
					temp_buf, strlen(temp_buf), 0);
		    fprintf(stderr, "Sent %d bytes to %s@%s\n", rlen,
			    temp_hostname, temp_portnum);
		} while(1);
	    }
	    if (msg[0] == 'r') {
		struct sockaddr temp_addr;
		char temp_buf[1024];
		int rlen;
		rlen = recv_message(sock, temp_buf, 1024, 0, &temp_addr);
		fprintf(stderr, "recved %d bytes = %s\n", rlen, temp_buf);
	    }
	    if (0 == strcmp(msg, "x")) {
		fprintf(stderr, "x pressed\n");
		break;
	    }
	}
	else if (FD_ISSET(sock, readfds)) {
	    struct sockaddr temp_addr;
	    char *temp_buf;
	    int rlen;
	    trans_pkt_t *incoming_pkt;
	    struct sockaddr_in forward_addr;
	    temp_buf = (char *) malloc(sizeof(trans_pkt_t));
	    rlen = recv_message(sock, temp_buf, sizeof(trans_pkt_t), 0, &temp_addr);
	    //fprintf(stderr, "recved %d bytes = %s\n", rlen, temp_buf);
	    incoming_pkt = (trans_pkt_t *) temp_buf;
	    switch(incoming_pkt->type) {
	    case PKT_TYPE_CONNECT_FTP2TCPD:
		fprintf(stderr, "Connect request packet from FTP has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		memcpy((incoming_pkt->payload), &temp_addr, sizeof(struct sockaddr));
		incoming_pkt->type = PKT_TYPE_CONNECT_TCPD2TCPD;
		forward_addr.sin_port = htons(12345);
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		break;
	    case PKT_TYPE_SEND_FTP2TCPD:
		fprintf(stderr, "Send request packet from FTP has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		forward_addr.sin_port = htons(12345);
		if (incoming_pkt->len == 128) {
		    char temp_name[128];
		    //int temp_size;
		    memcpy(temp_name, incoming_pkt->payload, 128);
		    fprintf(stderr, "seq num %d\n", incoming_pkt->seq_no);
		    //memcpy(&temp_size, (void *)((char *)&(incoming_pkt->payload) + 256), sizeof(int));
		    fprintf(stderr, "switch tcpd (%s)\n", temp_name);
		}
		if (incoming_pkt->len == 4) {
		    int temp_size;
		    fprintf(stderr, "seq num %d\n", incoming_pkt->seq_no);
		    memcpy(&temp_size, (void *)((char *)incoming_pkt->payload), sizeof(int));
		    fprintf(stderr, "switch tcpd (%d)\n", temp_size);
		}
		incoming_pkt->type = PKT_TYPE_SEND_TCPD2TCPD;
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		break;
	    case PKT_TYPE_CONNECT_TCPD2TCPD:
		fprintf(stderr, "Connect request packet from TCPD has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		incoming_pkt->type = PKT_TYPE_CONNECT_TCPD2FTP;
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		break;
	    case PKT_TYPE_SEND_TCPD2TCPD:
		fprintf(stderr, "Send request packet from TCPD has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		incoming_pkt->type = PKT_TYPE_SEND_TCPD2FTP;
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		break;
//	    case PKT_TYPE_CONNECT_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
//	    case PKT_TYPE_SEND_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
	    default:
		fprintf(stderr, "unexpected packet at tcpd\n");
		break;
	    }
	}
	else {
	    fprintf(stderr, "Time out\n");
	}
    }

    free_tcpd_addresses();
    free_fds();
    return 0;
}
