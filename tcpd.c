#include "tcpd.h"

// Initialize wrap-around buffers
int setup_tcpd_buffers()
{
    tcpd_window_head_index = 0;
    tcpd_window_tail_index = 0;
    tcpd_window_current_size = 0;
    return 0;
}

int get_free_buffer_index()
{
    int rval = tcpd_window_head_index;
    if  (tcpd_window_current_size== TCPD_POOL_SZ) {
	fprintf(stderr, "TCPD buffers full - need to wait for some buffers to free up\n");
    }
    tcpd_window_head_index = (tcpd_window_head_index + 1) % TCPD_POOL_SZ;
    tcpd_window_current_size++;
    return rval;
}

int release_buffer(int index)
{
    // verify what we're trying to release is in fact the one at the tail
    if (index != tcpd_window_tail_index) {
	fprintf(stderr, "Tail = %d, release_buffer index = %d", index, tcpd_window_tail_index);
    }
    tcpd_window_tail_index = (tcpd_window_tail_index + 1) % TCPD_POOL_SZ;
    tcpd_window_current_size--;
    return 0;
}

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
    int recv_sock = -1;
    int timer_sock = -1;
    fd_set readfds[4];
    char msg[1024];
    struct addrinfo *tcpd_addr;
    struct addrinfo *tcpd_addr_recv;
    struct addrinfo *timer_addrinfo;
    struct sockaddr *timer_addr;
    char timer_port_num_str[128];
    char timer_port_num_str_recv[128];
    char port_num_str_recv[128];
    char *port_num_str;

    // Check if arguments passed are correct
    if (argc < 2) {
	fprintf(stderr, "Usage: ./tcpd port-number");
	exit(-1);
    }
    else {
	port_num_str = argv[1];
	sprintf(port_num_str_recv, "%d", 12346);
	sprintf(timer_port_num_str, "%d", 12321);
	sprintf(timer_port_num_str_recv, "%d", 12322);
    }

    setup_tcpd_addr(SOCK_TYPE_DGRAM, NULL, port_num_str, &tcpd_addr);
    //setup_tcpd_buffers();
    sock = get_tcpd_sock(tcpd_addr);

    setup_tcpd_addr(SOCK_TYPE_DGRAM, NULL, port_num_str_recv, &tcpd_addr_recv);
    //setup_tcpd_buffers();
    recv_sock = get_tcpd_sock(tcpd_addr_recv);
    
    
    get_addr(SOCK_TYPE_DGRAM, NULL, timer_port_num_str, &timer_addrinfo);
    timer_addr = (struct sockaddr *) timer_addrinfo->ai_addr;
    
    get_addr(SOCK_TYPE_DGRAM, NULL, timer_port_num_str_recv, &timer_addrinfo);
    timer_sock = get_tcpd_sock(timer_addrinfo);
    
    tv.tv_sec = 40;
    tv.tv_usec = 0;

    while(1) {
	FD_ZERO(readfds);
	FD_SET(STDIN, readfds);
	FD_SET(sock, readfds);
	FD_SET(recv_sock, readfds);
	FD_SET(timer_sock, readfds);
	select(timer_sock + 1, readfds, NULL, NULL, &tv);

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
	if (FD_ISSET(sock, readfds)) {
	    struct sockaddr temp_addr;
	    char *temp_buf = NULL;
	    int rlen;
	    trans_pkt_t *incoming_pkt;
	    trans_pkt_t temp_pkt;
	    trans_pkt_t ack_pkt;
	    trans_pkt_t *timer_pkt = &temp_pkt;
	    int timeout_seq = -1;
	    int temp_int;
	    int rtt = 1000;
	    //int buffer_index;
	    struct sockaddr_in forward_addr;
	    struct sockaddr_in ack_addr;
	    temp_buf = (char *) malloc(sizeof(trans_pkt_t));
	    //buffer_index = get_free_buffer_index();
	    //temp_buf = &(buffer_pkt_pool[buffer_index]);
	    rlen = recv_message(sock, temp_buf, sizeof(trans_pkt_t), 0, &temp_addr);
	    //fprintf(stderr, "recved %d bytes = %s\n", rlen, temp_buf);
	    incoming_pkt = (trans_pkt_t *) temp_buf;
	    switch(incoming_pkt->type) {
	    case PKT_TYPE_CONNECT_FTP2TCPD:
		fprintf(stderr, "Connect request packet from FTP has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		memcpy((incoming_pkt->payload), &temp_addr, sizeof(struct sockaddr));
		incoming_pkt->type = PKT_TYPE_CONNECT_TCPD2TCPD;
		//changing the port address ensures that the packet is
		//targeted for the tcpd on the destination machine
		//rather than for the server itself
		forward_addr.sin_port = htons(12345);
		incoming_pkt->seq_no = current_seq_no;
		timer_pkt->type = PKT_TYPE_ADDNODE_TCPD2TIMER;
		memcpy(timer_pkt->payload, &current_seq_no, sizeof(int));
		memcpy((void *)((char *)timer_pkt->payload + sizeof(int)), &rtt, sizeof(int));
		current_seq_no = (current_seq_no + 1) % MAX_SEQ_NUM;
		fprintf(stderr, "Sending pkt number(%d)\n", incoming_pkt->seq_no);
		forward_message(sock, (struct sockaddr *)&forward_addr,
				temp_buf, sizeof(trans_pkt_t), 0);
		forward_message(sock, (struct sockaddr *)timer_addr,
				timer_pkt, sizeof(trans_pkt_t), 0);
		// while ack isn't received resend packet
		//while (ack_not_received) {
		//    wait_for_ack(round_trip_time);
		//}
		break;
	    case PKT_TYPE_SEND_FTP2TCPD:
		fprintf(stderr, "Send request packet from FTP has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		//changing the port address ensures that the packet is
		//targeted for the tcpd on the destination machine
		//rather than for the server itself
		forward_addr.sin_port = htons(12345);
		//if (0 && incoming_pkt->len == 128) {
		//    char temp_name[128];
		//    //int temp_size;
		//    memcpy(temp_name, incoming_pkt->payload, 128);
		//    fprintf(stderr, "seq num %d\n", incoming_pkt->seq_no);
		//    //memcpy(&temp_size, (void *)((char *)&(incoming_pkt->payload) + 256), sizeof(int));
		//    fprintf(stderr, "switch tcpd (%s)\n", temp_name);
		//}
		//if (0 && incoming_pkt->len == 4) {
		//    int temp_size;
		//    fprintf(stderr, "seq num %d\n", incoming_pkt->seq_no);
		//    memcpy(&temp_size, (void *)((char *)incoming_pkt->payload), sizeof(int));
		//    fprintf(stderr, "switch tcpd (%d)\n", temp_size);
		//}
		incoming_pkt->type = PKT_TYPE_SEND_TCPD2TCPD;
		incoming_pkt->seq_no = current_seq_no;
		timer_pkt->type = PKT_TYPE_ADDNODE_TCPD2TIMER;
		memcpy(timer_pkt->payload, &current_seq_no, sizeof(int));
		memcpy((void *)((char *)timer_pkt->payload + sizeof(int)), &rtt, sizeof(int));
		current_seq_no = (current_seq_no + 1) % MAX_SEQ_NUM;
		fprintf(stderr, "Sending pkt number(%d)\n", incoming_pkt->seq_no);
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		forward_message(sock, (struct sockaddr *)timer_addr,
				timer_pkt, sizeof(trans_pkt_t), 0);
		break;
	    case PKT_TYPE_CONNECT_TCPD2TCPD:
		fprintf(stderr, "Connect request packet from TCPD has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		incoming_pkt->type = PKT_TYPE_CONNECT_TCPD2FTP;
		if (next_expected_seq_no == incoming_pkt->seq_no) {
		    fprintf(stderr, "Received pkt %d as expected\n", incoming_pkt->seq_no);
		    // send out ack
		    ack_pkt.type = PKT_TYPE_ACK_TCPD2TCPD;
		    temp_int = next_expected_seq_no + 1;
		    memcpy(ack_pkt.payload, &temp_int, sizeof(int));
		    memcpy(&ack_addr, &temp_addr, sizeof(struct sockaddr));
		    ack_addr.sin_port = htons(12346);
		    forward_message(sock, (struct sockaddr *)&ack_addr,
				    &ack_pkt, sizeof(trans_pkt_t), 0);
		}
		else {
		    fprintf(stderr, "Received pkt %d but expected %d\n", incoming_pkt->seq_no,
			    next_expected_seq_no);
		    exit(-1);
		}
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		next_expected_seq_no = (next_expected_seq_no + 1) % MAX_SEQ_NUM;
		break;
	    case PKT_TYPE_SEND_TCPD2TCPD:
		fprintf(stderr, "Send request packet from TCPD has arrived at TCPD\n");
		memcpy(&forward_addr, &(incoming_pkt->addr), sizeof(struct sockaddr));
		incoming_pkt->type = PKT_TYPE_SEND_TCPD2FTP;
		if (next_expected_seq_no == incoming_pkt->seq_no) {
		    fprintf(stderr, "Received pkt %d as expected\n", incoming_pkt->seq_no);
		    // send out ack
		    ack_pkt.type = PKT_TYPE_ACK_TCPD2TCPD;
		    temp_int = next_expected_seq_no + 1;
		    memcpy(ack_pkt.payload, &temp_int, sizeof(int));
		    memcpy(&ack_addr, &temp_addr, sizeof(struct sockaddr));
		    ack_addr.sin_port = htons(12346);
		    forward_message(sock, (struct sockaddr *)&ack_addr,
				    &ack_pkt, sizeof(trans_pkt_t), 0);
		}
		else {
		    fprintf(stderr, "Received pkt %d but expected %d\n", incoming_pkt->seq_no,
			    next_expected_seq_no);
		    exit(-1);
		}
		forward_message(sock, (struct sockaddr *)&forward_addr,
					temp_buf, sizeof(trans_pkt_t), 0);
		next_expected_seq_no = (next_expected_seq_no + 1) % MAX_SEQ_NUM;
		break;
	    case PKT_TYPE_TIMEOUT_TIMER2TCPD:
		memcpy(&timeout_seq, (incoming_pkt->payload), sizeof(int));
		fprintf(stderr, "timedout seq num %d - need to resend\n", timeout_seq);
		break;
//	    case PKT_TYPE_CONNECT_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
//	    case PKT_TYPE_SEND_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
	    default:
		fprintf(stderr, "unexpected packet at tcpd sock\n");
		break;
	    }
	    // need to release buffers to pool after use
	    if (NULL != temp_buf) {
	    	free(temp_buf);
	    }
	}
	if (FD_ISSET(recv_sock, readfds)) {
	    struct sockaddr temp_addr;
	    char *temp_buf = NULL;
	    int rlen;
	    trans_pkt_t *incoming_pkt;
	    trans_pkt_t ack_pkt;
	    //int timeout_seq = -1;
	    int temp_int;
	    //int rtt = 1000;
	    //int buffer_index;
	    //struct sockaddr_in forward_addr;
	    //buffer_index = get_free_buffer_index();
	    //temp_buf = &(buffer_pkt_pool[buffer_index]);
	    rlen = recv_message(recv_sock, &ack_pkt, sizeof(trans_pkt_t), 0, &temp_addr);
	    //fprintf(stderr, "recved %d bytes = %s\n", rlen, temp_buf);
	    incoming_pkt = (trans_pkt_t *) &ack_pkt;
	    switch(incoming_pkt->type) {
	    case PKT_TYPE_ACK_TCPD2TCPD:
		memcpy(&temp_int, incoming_pkt->payload, sizeof(int));
		fprintf(stderr, "Ack packet (for seq = %d) from TCPD has arrived\n", temp_int);
		break;
//	    case PKT_TYPE_CONNECT_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
//	    case PKT_TYPE_SEND_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
	    default:
		fprintf(stderr, "unexpected packet at tcpd recv sock\n");
		break;
	    }
	    // need to release buffers to pool after use
	    if (NULL != temp_buf) {
	    	free(temp_buf);
	    }
	}
	if (FD_ISSET(timer_sock, readfds)) {
	    struct sockaddr temp_addr;
	    char *temp_buf = NULL;
	    int rlen;
	    trans_pkt_t *incoming_pkt;
	    //trans_pkt_t temp_pkt;
	    //trans_pkt_t *timer_pkt = &temp_pkt;
	    int timeout_seq = -1;
	    //int rtt = 1000;
	    //int buffer_index;
	    //struct sockaddr_in forward_addr;
	    temp_buf = (char *) malloc(sizeof(trans_pkt_t));
	    //buffer_index = get_free_buffer_index();
	    //temp_buf = &(buffer_pkt_pool[buffer_index]);
	    rlen = recv_message(timer_sock, temp_buf, sizeof(trans_pkt_t), 0, &temp_addr);
	    //fprintf(stderr, "recved %d bytes = %s\n", rlen, temp_buf);
	    incoming_pkt = (trans_pkt_t *) temp_buf;
	    switch(incoming_pkt->type) {
	    case PKT_TYPE_TIMEOUT_TIMER2TCPD:
		memcpy(&timeout_seq, (incoming_pkt->payload), sizeof(int));
		fprintf(stderr, "timedout seq num %d - need to resend\n", timeout_seq);
		break;
//	    case PKT_TYPE_CONNECT_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
//	    case PKT_TYPE_SEND_TCPD2FTP:
//		fprintf(stderr, "Connect request packet from TCPD has arrived at FTP\n");
//		break;
	    default:
		fprintf(stderr, "unexpected packet at tcpd timer sock\n");
		break;
	    }
	    // need to release buffers to pool after use
	    if (NULL != temp_buf) {
	    	free(temp_buf);
	    }
	}
	else {
	    fprintf(stderr, "Time out\n");
	    tv.tv_sec = 40;
	    tv.tv_usec = 0;
	}
    }

    free_tcpd_addresses();
    free_fds();
    return 0;
}
