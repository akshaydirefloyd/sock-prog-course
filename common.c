#include "common.h"
#include "sock_wrapper_api.h"

#define MAX_ADDR_LIST 20
#define MAX_SOCK_LIST 20

struct addrinfo **addr_list;
int sock_list[MAX_SOCK_LIST];
int addr_list_count = 0;
int sock_list_count = 0;

int init_all()
{
    //initialize some basic things
    init_socks_api();
}

// Setup address for either server or client
int setup_addr(int sock_type, char *node_name, char *port_num_str, struct addrinfo **ret_addr)
{
    struct addrinfo *res, *res_ptr_iter, hints;
    char ip_addr_str[128];
    struct sockaddr_in *ip_sock_addr;
    
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   //Need IPV4
    hints.ai_flags = AI_PASSIVE; //Free to choose any of the local interfaces
    if (sock_type == SOCK_TYPE_STREAM) {
	hints.ai_socktype = SOCK_STREAM; //Need TCP sockets
    }
    else {
	hints.ai_socktype = SOCK_DGRAM; //Need UDP sockets
    }

    if (0 != getaddrinfo(node_name, port_num_str, &hints, &res)) {
	fprintf(stderr, "Error in getaddrinfo: %s\n", strerror(errno));
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
    if (0 == addr_list_count) {
	addr_list = (struct addrinfo **) malloc(sizeof(struct addrinfo *) * MAX_ADDR_LIST);
    }
    
    addr_list[addr_list_count] = res;
    addr_list_count++;
    
    return 0;
}

// Free all initiated addrinfo structures
int free_addresses()
{
    while (0 != addr_list_count) {
	addr_list_count--;
	freeaddrinfo(addr_list[addr_list_count]);
    }
    free(addr_list);
    return 0;
}

// create a socket that is connected to the remote server
int setup_client_cxn(struct addrinfo *addr)
{
    int sending_socket = -1;
    
    sending_socket = socket(addr->ai_family, addr->ai_socktype, 
			      addr->ai_protocol);
    if (-1 == sending_socket) {
	fprintf(stderr, "Error in socket creation:%s\n", strerror(errno));
	exit(-1);
    }

    /*
    if (-1 == bind(sending_socket, addr->ai_addr, 
		   addr->ai_addrlen)) {
	fprintf(stderr, "Error in bind:%s\n", strerror(errno));
	exit(-1);
    }
    */
    
    if (-1 == connect(sending_socket, addr->ai_addr, 
		      addr->ai_addrlen)) {
	fprintf(stderr, "Error in connect:%s\n", strerror(errno));
	exit(-1);
    }
    else {
	fprintf(stderr, "Connected to server\n");
    }

    // Keep track of socket opened for freeing later
    sock_list[sock_list_count++] = sending_socket;
    
    return sending_socket;
}

// create a socket that is capable of listening at the specified addr
int setup_server_cxn(struct addrinfo *addr, int *remote_socket, char *remote_node_name)
{
    int listening_socket = -1;
    int backlog = 10;
    struct sockaddr_storage remote_addr;
    socklen_t remote_addr_len;
    remote_addr_len = sizeof(struct sockaddr_storage);
    char ip_addr_str[128];
    struct sockaddr_in *ip_sock_addr;
    
    char remote_port_num[PORT_NUM_STR_MAX_LEN];
    
    listening_socket = socket(addr->ai_family, addr->ai_socktype, 
			      addr->ai_protocol);
    if (-1 == listening_socket) {
	fprintf(stderr, "Error in socket creation:%s\n", strerror(errno));
	exit(-1);
    }

    if (-1 == bind(listening_socket, addr->ai_addr, 
		   addr->ai_addrlen)) {
	fprintf(stderr, "Error in bind:%s\n", strerror(errno));
	exit(-1);
    }

    if (-1 == listen(listening_socket, backlog)) {
	fprintf(stderr, "Error in listen:%s\n", strerror(errno));
	exit(-1);
    }

    *remote_socket = accept(listening_socket, (struct sockaddr *)&remote_addr, 
			   &remote_addr_len);
    if (-1 == *remote_socket) {
	fprintf(stderr, "Error in accept:%s\n", strerror(errno));
	exit(-1);
    }
    else {
	ip_sock_addr = (struct sockaddr_in *) &remote_addr;
	if (NULL == inet_ntop(addr->ai_family, &(ip_sock_addr->sin_addr), 
			      ip_addr_str, sizeof(ip_addr_str))) {
	    fprintf(stderr, "Error in inet_ntop : %s\n", strerror(errno));
	    exit(-1);
	}
	fprintf(stderr, "Accepted address from %s\n", ip_addr_str);

	if (0 != getnameinfo((struct sockaddr *) &remote_addr, sizeof(struct sockaddr), 
			     remote_node_name, REMOTE_HOST_STR_MAX_LEN,
			     remote_port_num, PORT_NUM_STR_MAX_LEN, NI_NAMEREQD)) {
	    fprintf(stderr, "Error in getnameinfo : %s\n", strerror(errno));
	    exit(-1);
	}
	fprintf(stderr, "Remote host = %s, remote port = %s\n", remote_node_name, remote_port_num);
    }

    // Keep track of socket opened for freeing later
    sock_list[sock_list_count++] = listening_socket;
    sock_list[sock_list_count++] = *remote_socket;
    
    return listening_socket;
}

// Free all initiated addrinfo structures
int free_sockets()
{
    while (0 != sock_list_count) {
	sock_list_count--;
	close(sock_list[sock_list_count]);
    }
    return 0;
}

// send header information to server
int send_header(int sending_socket, header_msg_t my_file_header)
{
    int total_buf_len = 0;
    int buf_len_sent = 0;
    int send_out = -1;
    int buf_seg_sz = SEND_BUF_SZ / 4;
    
    total_buf_len = sizeof(header_msg_t);
    buf_len_sent = 0;
    while (buf_len_sent < total_buf_len) {
	buf_seg_sz = (buf_seg_sz > (total_buf_len - buf_len_sent)) ?
	    buf_seg_sz : (total_buf_len - buf_len_sent);
	send_out = send(sending_socket, (void *)((char *)&my_file_header + buf_len_sent), buf_seg_sz, 0);
	if (-1 == send_out) {
	    fprintf(stderr, "Error in send:%s\n", strerror(errno));
	    exit(-1);
	}
	buf_len_sent += send_out;
	fprintf(stderr, "sent %d bytes\n", send_out);
    }
    fprintf(stderr, "header exchange done\n");
    return 0;
}

// recv header information at server
header_msg_t recv_header(int remote_socket)
{
    int buf_seg_sz = RECV_BUF_SZ / 4;
    int total_buf_len = 0;
    int buf_len_recvd = 0;
    int recv_out = -1;
    header_msg_t remote_file_header;
    
    total_buf_len = sizeof(header_msg_t);
    buf_len_recvd = 0;
    
    while (buf_len_recvd < total_buf_len) {
	buf_seg_sz = (buf_seg_sz >= (total_buf_len - buf_len_recvd)) ? 
	    buf_seg_sz : (total_buf_len - buf_len_recvd);
	recv_out = recv(remote_socket, (void *)((char *)&remote_file_header + buf_len_recvd), buf_seg_sz, 0);
	if (-1 == recv_out) {
	    fprintf(stderr, "Error in recv:%s\n", strerror(errno));
	    exit(-1);
	}
	buf_len_recvd += recv_out;
	fprintf(stderr, "received %d bytes\n", recv_out);
    }
    return remote_file_header;
}

// send file to server
int send_file(int sending_socket, header_msg_t my_file_header, int my_file_fd)
{
    int total_buf_len = 0;
    int buf_len_sent = 0;
    int buf_len_read = 0;
    int send_out = -1;
    ssize_t file_size_offset = -1;
    int buf_seg_sz = SEND_BUF_SZ / 4;
    char send_buf[SEND_BUF_SZ];
    
    //Initialize some data and then send it
    buf_seg_sz = SEND_BUF_SZ / 4;
    total_buf_len = my_file_header.file_size;
    
    buf_len_sent = 0;
    while (buf_len_sent < total_buf_len) {
	buf_seg_sz = (buf_seg_sz > (total_buf_len - buf_len_sent)) ?
	    (total_buf_len - buf_len_sent) : buf_seg_sz;
	buf_len_read = 0;
	//Make sure that all of buf_seg_sz data has been read into the send buf
	while (buf_len_read < buf_seg_sz) {
	    file_size_offset = read(my_file_fd, (void *)((char *)send_buf + buf_len_read), (buf_seg_sz - buf_len_read));
	    if (-1 == file_size_offset) {
		fprintf(stderr, "Error in read(%s)\n", strerror(errno));
		exit(-1);
	    }
	    buf_len_read += file_size_offset;
	    fprintf(stderr, "Done reading %d bytes (%d, %d)\n", (int) file_size_offset, buf_seg_sz, buf_len_read);
	}
	//send_out = send(sending_socket, (void *)((char *)send_buf + buf_len_sent), buf_seg_sz, 0);
	send_out = send(sending_socket, (void *)((char *)send_buf), buf_seg_sz, 0);
	if (-1 == send_out) {
	    fprintf(stderr, "Error in send:%s\n", strerror(errno));
	    exit(-1);
	}
	buf_len_sent += send_out;
	fprintf(stderr, "sent %d bytes\n", send_out);
    }
    return 0;
}

// recv file at server
int recv_file(int remote_socket, int remote_file_fd, header_msg_t remote_file_header)
{
    char recv_buf[RECV_BUF_SZ];
    int buf_seg_sz = RECV_BUF_SZ / 4;
    int total_buf_len = 0;
    int buf_len_recvd = 0;
    int buf_len_written = 0;
    int recv_out = -1;
    off_t file_offset = 0;
    ssize_t file_size_offset = -1;
    
    total_buf_len = remote_file_header.file_size;
    buf_seg_sz = RECV_BUF_SZ / 4;
    
    buf_len_recvd = 0;
    while (buf_len_recvd < total_buf_len) {
	buf_seg_sz = (buf_seg_sz >= (total_buf_len - buf_len_recvd)) ? 
	    (total_buf_len - buf_len_recvd) : buf_seg_sz;
	//recv_out = recv(remote_socket, (void *)((char *)recv_buf + buf_len_recvd), buf_seg_sz, 0);
	recv_out = recv(remote_socket, (void *)((char *)recv_buf), buf_seg_sz, 0);
	if (-1 == recv_out) {
	    fprintf(stderr, "Error in recv:%s\n", strerror(errno));
	    exit(-1);
	}
	buf_len_recvd += recv_out;
	fprintf(stderr, "received %d bytes\n", recv_out);
	//Write whatever received into file
	buf_len_written = 0;
	file_offset = lseek(remote_file_fd, (off_t) 0, SEEK_END);
	if (-1 == file_offset) {
	    fprintf(stderr, "Error in lseek:%s\n", strerror(errno));
	    exit(-1);
	}
	while (buf_len_written < recv_out) {
	    //file_offset = lseek(remote_file_fd, (off_t) 0, SEEK_END);
	    //if (-1 == file_offset) {
	    //	fprintf(stderr, "Error in lseek:%s\n", strerror(errno));
	    //	exit(-1);
	    //}
	    file_size_offset = write(remote_file_fd, (void *)((char *)recv_buf + buf_len_written), 
				     (recv_out - buf_len_written));
	    if (-1 == file_size_offset) {
		fprintf(stderr, "Error in write:%s\n", strerror(errno));
		exit(-1);
	    }
	    buf_len_written += file_size_offset;
	}
    }
    return 0;
}

int create_client_file(char *remote_node_name, header_msg_t remote_file_header)
{
    char remote_file_name[REMOTE_FILE_NAME_MAX_LEN];
    int remote_file_fd = -1;
    
    sprintf(remote_file_name, "/tmp/%s", remote_node_name);
    
    if (-1 == mkdir(remote_file_name, 0777)) {
	if (EEXIST == errno) {
	}
	else {
	    fprintf(stderr, "Error in mkdir:%s\n", strerror(errno));
	    exit(-1);
	}
    }
    
    sprintf(remote_file_name, "/tmp/%s/%s", remote_node_name, remote_file_header.file_name);
    fprintf(stderr, "header exchange done. Storing in %s (size = %d)\n", 
	    remote_file_name, remote_file_header.file_size);

    //open file for writing
    remote_file_fd = open(remote_file_name, (O_RDWR | O_CREAT | O_APPEND),
			  (S_IRWXU | S_IRWXG| S_IRWXO));
    if (-1 == remote_file_fd) {
	fprintf(stderr, "Error in open:%s\n", strerror(errno));
	exit(-1);
    }
    return remote_file_fd;
}
