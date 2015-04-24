/*
  Timer process is in charge of receiving messages from tcpd which may
  start a timer or cancel a timer. For each request to create a timer
  a node is created and added to the queue and conversely removed from
  the queue if a cancel is requested. Nodes are identified by the
  sequence number. 
 */
#include "timer.h"

int init_delta_list()
{
    int i = 0;
    timeout_node_t *ref;
    delta_list = malloc(sizeof(timeout_node_t) * DELTA_LENGTH);
    ref = delta_list;
    ref[0].next = &ref[1];
    ref[0].prev = NULL;
    ref[0].seq_no = -1;
    ref[0].timeout_duration = -1;
    ref[0].index = 0;
    for (i = 1; i < (DELTA_LENGTH - 1); i++) {
	ref[i].next = &ref[i + 1];
	ref[i].prev = &ref[i - 1];
	ref[i].seq_no = -1;
	ref[i].timeout_duration = -1;
	ref[i].index = i;
    }
    ref[i].index = i;
    ref[i].next = NULL;
    ref[i].prev = &ref[i - 1];
    ref[i].seq_no = -1;
    ref[i].timeout_duration = -1;
    delta_list_head_free = &ref[0];
    delta_list_tail_free = &ref[DELTA_LENGTH - 1];
    delta_list_head_used = NULL;
    delta_list_tail_used = NULL;
}

int add_node(int seq_no, int timeout_duration)
{
    timeout_node_t *ref;
    timeout_node_t *ref2;
    timeout_node_t *ref_prev;
    timeout_node_t *ref_next;
    int time_elapsed = 0;
    struct timeval now;
    int sum = 0;
    if (used_list_count == DELTA_LENGTH) {
	fprintf(stderr, "All nodes used up\n");
	exit(-1);
    }
    if (NULL == delta_list_head_used) {
	// nothing in used list. Pull out the head of free list and
	// make the head and tail of used list. The head and tail of
	// used list will naturally point to NULLs being the only
	// element
	fprintf(stderr, "First element added\n");
	delta_list_head_used = delta_list_head_free;
	delta_list_tail_used = delta_list_head_free;
	ref = delta_list_head_free->next;
	delta_list_head_used->next = NULL;
	delta_list_head_used->prev = NULL;
	delta_list_head_free = ref;
	delta_list_head_used->seq_no = seq_no;
	delta_list_head_used->timeout_duration = timeout_duration;
	gettimeofday(&(delta_list_head_used->start_time), NULL);
    }
    else {
	// Pull out the head of free list and attach to some point in
	// the used list
	// there is at least one element in used list and free
	// list. If the first element has timeout duration that's
	// greater than the new entry there is a need to cancel and
	// place it behind the new entry
	ref = delta_list_head_free;
	ref->seq_no = seq_no;
	if (1 == free_list_count) {
	    delta_list_head_free = NULL;
	    delta_list_tail_free = NULL;
	}
	else {
	    delta_list_head_free = ref->next;
	}
	gettimeofday(&now, NULL);
	ref2 = delta_list_head_used;
	time_elapsed = (now.tv_usec - (ref2->start_time).tv_usec)
	    + (1000000 * (now.tv_sec - (ref2->start_time).tv_sec));
	time_elapsed = 0;
	sum = 0 - time_elapsed;
	do {
	    sum += ref2->timeout_duration;
	    fprintf(stderr, "inspected index %d in used list sum = %d\n", ref2->index, sum);
	    if (timeout_duration < sum) {
		break;
	    }
	    ref2 = ref2->next;
	} while (ref2 != NULL);

	if (ref2 != NULL) {
	    ref_next = ref2->next;
	    ref_prev = ref2->prev;

	    if (NULL == ref_prev) {
		fprintf(stderr, "attaching node to head\n");
		ref->next = ref2;
		ref->prev = NULL;
		ref2->prev = ref;
		delta_list_head_used = ref;
		ref2->timeout_duration = sum - timeout_duration;
		ref->timeout_duration = timeout_duration;
	    }
	    else {
		fprintf(stderr, "attaching node in between\n");
		sum -= ref2->timeout_duration;
		ref2->prev = ref;
		ref_prev->next = ref;
		ref->prev = ref_prev;
		ref->next = ref2;
		ref->timeout_duration = timeout_duration - sum;
		ref2->timeout_duration -= ref->timeout_duration;
		//ref2->timeout_duration = sum - timeout_duration;
	    }
	}
	else {
	    fprintf(stderr, "attaching node to tail\n");
	    ref->prev = delta_list_tail_used;
	    ref->next = NULL;
	    delta_list_tail_used->next = ref;
	    delta_list_tail_used = ref;
	    ref->timeout_duration = timeout_duration - sum;
	}
    }
    used_list_count++;
    free_list_count--;
    fprintf(stderr, "Used = %d, Free = %d\n", used_list_count, free_list_count);
}

int free_node(int seq_no)
{
    int match_found = 0;
    timeout_node_t *ref;
    timeout_node_t *ref_prev;
    timeout_node_t *ref_next;
    
    ref = delta_list_head_used;
    
    while(NULL != ref) {
	if (seq_no == ref->seq_no) {
	    match_found = 1;
	    // remove from used list and put it back to free list
	    if (free_list_count == 0) {
		delta_list_head_free = ref;
		delta_list_tail_free = ref;
		ref_next = ref->next;
		ref_prev = ref->prev;
		ref->next = NULL;
		ref->prev = NULL;

		if (NULL != ref_prev && NULL != ref_next) {
		    // node not at the extremeties so no changes to
		    // head or tail
		    ref_prev->next = ref_next;
		    ref_next->prev = ref_prev;
		    ref_next->timeout_duration += ref->timeout_duration;
		}
		else if (NULL != ref_prev && NULL == ref_next) {
		    // tail node being removed
		    ref_prev->next = NULL;
		    delta_list_tail_used = ref_prev;
		}
		else if (NULL == ref_prev && NULL != ref_next) {
		    // head node being removed
		    ref_next->prev = NULL;
		    delta_list_head_used = ref_next;
		}
		else if (NULL == ref_prev && NULL == ref_next) {
		    // this would happen if ref is the last element in used list
		    fprintf(stderr, "free_node() unexpected case used list length cannot be 1 while free list len is aero\n");
		}
		else {
		    fprintf(stderr, "free_node() unexpected case\n");
		    exit(-1);
		}
	    }
	    else {
		// there is at least one element in free and used list
		if (1 == used_list_count) {
		    // ref is the last element in used list
		    delta_list_head_used->next = NULL;
		    delta_list_head_used->prev = delta_list_tail_free;
		    delta_list_tail_free->next = delta_list_head_used;
		    delta_list_head_used = NULL;
		    delta_list_tail_used = NULL;
		}
		else {
		    ref_next = ref->next;
		    ref_prev = ref->prev;

		    if (NULL != ref_prev && NULL != ref_next) {
			// node not at the extremeties so no changes to
			// head or tail
			ref_prev->next = ref_next;
			ref_next->prev = ref_prev;
			ref_next->timeout_duration += ref->timeout_duration;
		    }
		    else if (NULL != ref_prev && NULL == ref_next) {
			// tail node being removed
			ref_prev->next = NULL;
			delta_list_tail_used = ref_prev;
		    }
		    else if (NULL == ref_prev && NULL != ref_next) {
			// head node being removed
			ref_next->prev = NULL;
			delta_list_head_used = ref_next;
		    }
		    else if (NULL == ref_prev && NULL == ref_next) {
			// this would happen if ref is the last element in used list
			fprintf(stderr, "free_node() unexpected case rp and rn are null\n");
		    }
		    else {
			fprintf(stderr, "free_node() unexpected case\n");
			exit(-1);
		    }
		    ref->next = NULL;
		    ref->prev = delta_list_tail_free;
		    delta_list_tail_free->next = ref;
		    delta_list_tail_free = ref;
		}
	    }
	    ref->seq_no = -1;
	    ref->timeout_duration = -1;
	    used_list_count--;
	    free_list_count++;
	    break;
	}
	ref = ref->next;
    }
    if (match_found == 1) {
	fprintf(stderr, "node removed from used list\n");
    }
    else {
	fprintf(stderr, "node not found\n");
    }
}

int print_lists()
{
    int i = 0;
    timeout_node_t *ref = delta_list_head_free;
    fprintf(stderr, "Free list (%d)\n", DELTA_LENGTH);
    while (NULL != ref) {
	fprintf(stderr, "%d, (%d, %d, %d)\n", i++, ref->index, ref->seq_no, ref->timeout_duration);
	ref = ref->next;
    }
    fprintf(stderr, "Used list\n");
    ref = delta_list_head_used;
    while (NULL != ref) {
	fprintf(stderr, "%d, (%d, %d, %d)\n", i++, ref->index, ref->seq_no, ref->timeout_duration);
	ref = ref->next;
    }
}
ssize_t send_message(int sock, struct sockaddr *addr, const void *buf,
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

int setup_timer_addr(int sock_type, char *node_name, char *port_num_str, struct addrinfo **ret_addr)
{
    struct addrinfo *res_ptr_iter, hints;
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
    return 0;
}

int setup_lb_addr(int sock_type, char *node_name, char *port_num_str, struct addrinfo **ret_addr)
{
    struct addrinfo *res_ptr_iter, hints;
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

    rval = getaddrinfo(node_name, port_num_str, &hints, &res_lb);
    if (0 != rval) {
	fprintf(stderr, "Error in getaddrinfo: %s, rval = %d port_num = %s\n",
		strerror(errno), rval, port_num_str);
	exit(-1);
    }

    //Go through valid address list and pick out first match
    //The following iteration isn't really necessary
    for (res_ptr_iter = res_lb; res_ptr_iter != NULL; 
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
    return 0;
}

int get_timer_sock(struct addrinfo *addr)
{
    int timer_socket = -1;
    
    timer_socket = socket(addr->ai_family, addr->ai_socktype, 
			      addr->ai_protocol);
    if (-1 == timer_socket) {
	fprintf(stderr, "Error in socket creation:%s\n", strerror(errno));
	exit(-1);
    }

    if (-1 == bind(timer_socket, addr->ai_addr, 
		   addr->ai_addrlen)) {
	fprintf(stderr, "Error in bind:%s\n", strerror(errno));
	exit(-1);
    }
    
    return timer_socket;
}

int main(int argc, char **argv)
{
    struct timeval tv;
    fd_set readfds[3];
    char msg[1024];
    struct addrinfo *timer_addr;
    struct addrinfo *lb_addr;
    int timer_sock = -1;
    int lb_sock = -1;
    char *port_num_str;
    char lb_port_num_str[256];
    struct sockaddr tcpd_addr;
    int tcpd_addr_set = 0;

    // Check if arguments passed are correct
    if (argc < 2) {
	fprintf(stderr, "Usage: ./timer port-number");
	exit(-1);
    }
    else {
	port_num_str = argv[1];
	sprintf(lb_port_num_str, "%d", 22222);
    }
    
    init_delta_list();
    print_lists();
    setup_timer_addr(SOCK_TYPE_DGRAM, NULL, port_num_str, &timer_addr);
    timer_sock = get_timer_sock(timer_addr);

    setup_lb_addr(SOCK_TYPE_DGRAM, NULL, lb_port_num_str, &lb_addr);
    lb_sock = get_timer_sock(lb_addr);
    
    tv.tv_sec = 200000;
    tv.tv_usec = 2;

    while(1) {
	FD_ZERO(readfds);
	FD_SET(STDIN, readfds);
	FD_SET(timer_sock, readfds);
	FD_SET(lb_sock, readfds);
	select(lb_sock + 1, readfds, NULL, NULL, &tv);

	if (FD_ISSET(STDIN, readfds)) {
	    fprintf(stderr, "key pressed\n");
	    scanf("%s", msg);
	    fprintf(stderr, "%s\n", msg);
	    if (0 == strcmp(msg, "x")) {
		fprintf(stderr, "x pressed\n");
		break;
	    }
	    else if (0 == strcmp(msg, "a")) {
		int seq_no;
		int timeout_duration;
		fprintf(stderr, "let's add nodes\n");
		fprintf(stderr, "enter seq num\n");
		scanf("%d", &seq_no);
		fprintf(stderr, "enter timeout duration\n");
		scanf("%d", &timeout_duration);
		fprintf(stderr, "Will attempt adding (%d, %d)\n", seq_no, timeout_duration);
		add_node(seq_no, timeout_duration);
		print_lists();
	    }
	    else if (0 == strcmp(msg, "d")) {
		int seq_no;
		fprintf(stderr, "let's delete nodes\n");
		fprintf(stderr, "enter seq num\n");
		scanf("%d", &seq_no);
		fprintf(stderr, "Will attempt removing seq_no =%d\n", seq_no);
		free_node(seq_no);
		print_lists();
	    }
	    else if (0 == strcmp(msg, "l")) {
		struct sockaddr temp_addr;
		struct sockaddr_in *temp_addr_in;
		char *temp_buf = NULL;
		int rlen;
		trans_pkt_t *incoming_pkt;
		struct sockaddr_in forward_addr;
		temp_buf = (char *) malloc(sizeof(trans_pkt_t));
		incoming_pkt = (trans_pkt_t *) temp_buf;
		incoming_pkt->type = PKT_TYPE_LOOPBACK_TIMER2TIMER;
		send_message(timer_sock, (struct sockaddr *)lb_addr->ai_addr,
			     temp_buf, sizeof(trans_pkt_t), 0);
	    }
	}
	else if (FD_ISSET(timer_sock, readfds)) {
	    struct sockaddr temp_addr;
	    struct sockaddr_in *temp_addr_in;
	    char *temp_buf = NULL;
	    int rlen;
	    trans_pkt_t *incoming_pkt;
	    struct sockaddr_in forward_addr;
	    int *temp_int;
	    int seq_no;
	    int timeout_duration;
	    temp_buf = (char *) malloc(sizeof(trans_pkt_t));
	    rlen = recv_message(timer_sock, temp_buf, sizeof(trans_pkt_t), 0, &temp_addr);
	    temp_addr_in = (struct sockaddr_in *) &temp_addr;
	    if (12345 == ntohs(temp_addr_in->sin_port)) {
		if (0 == tcpd_addr_set) {
		    tcpd_addr = temp_addr;
		    tcpd_addr_set = 1;
		    fprintf(stderr, "TCPD Address is set\n");
		}
	    }
	    //fprintf(stderr, "recved %d bytes = %s\n", rlen, temp_buf);
	    incoming_pkt = (trans_pkt_t *) temp_buf;
	    switch(incoming_pkt->type) {
	    case PKT_TYPE_ADDNODE_TCPD2TIMER:
		// Request to add timer for a packet
		// parse payload (seq_no, time)
		temp_int = (int *)incoming_pkt->payload;
		seq_no = temp_int[0];
		timeout_duration = temp_int[1]; // in microsecs?
		// add_node();
		break;
	    case PKT_TYPE_DELNODE_TCPD2TIMER:
		// Request to cancel timer for a packet
		// parse payload (seq_no)
		temp_int = (int *)incoming_pkt->payload;
		seq_no = temp_int[0];
		// free_node();
		break;
	    default:
		fprintf(stderr, "unexpected packet at tcpd\n");
		break;
	    }
	    // need to release buffers to pool after use
	    if (NULL != temp_buf) {
	    	free(temp_buf);
	    }
	}
	else if (FD_ISSET(lb_sock, readfds)) {
	    struct sockaddr temp_addr;
	    struct sockaddr_in *temp_addr_in;
	    char *temp_buf = NULL;
	    int rlen;
	    trans_pkt_t *incoming_pkt;
	    struct sockaddr_in forward_addr;
	    int *temp_int;
	    int seq_no;
	    int timeout_len;
	    temp_buf = (char *) malloc(sizeof(trans_pkt_t));
	    rlen = recv_message(lb_sock, temp_buf, sizeof(trans_pkt_t), 0, &temp_addr);
	    incoming_pkt = (trans_pkt_t *) temp_buf;
	    if (PKT_TYPE_LOOPBACK_TIMER2TIMER == incoming_pkt->type) {
		fprintf(stderr, "Loopback executed\n");
	    }
	}
	else {
	    fprintf(stderr, "Time out\n");
	}
    }

    // free resources
    if (NULL != res) {
	freeaddrinfo(res);
	freeaddrinfo(res_lb);
    }
    if (-1 != timer_sock) {
	close(timer_sock);
	close(lb_sock);
    }
    
    if (NULL != delta_list) {
	free(delta_list);
    }
    return 0;
}

