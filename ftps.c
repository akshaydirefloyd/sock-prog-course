/*
 Server code which accepts an incoming file and stores it in a folder
 identified by the hostname where the file came from.
 */

#include "common.h"

void print_usage()
{
    fprintf(stderr, "./ftps local-port\n");
}

int main(int argc, char **argv)
{
    int listening_port = -1;
    int listening_socket = -1;
    int backlog = 10;
    char node_name[LOCAL_HOST_STR_MAX_LEN];
    char remote_node_name[REMOTE_HOST_STR_MAX_LEN];
    char remote_port_num[PORT_NUM_STR_MAX_LEN];
    char port_num_str[PORT_NUM_STR_MAX_LEN];
    char remote_file_name[REMOTE_FILE_NAME_MAX_LEN];
    struct addrinfo *res, *res_ptr_iter, hints;
    char ip_addr_str[128];
    struct sockaddr_in *ip_sock_addr;
    struct sockaddr_storage remote_addr;
    int remote_socket;
    socklen_t remote_addr_len = sizeof(struct sockaddr_storage);
    char recv_buf[RECV_BUF_SZ];
    int buf_seg_sz = RECV_BUF_SZ / 4;
    int total_buf_len = 0;
    int buf_len_recvd = 0;
    int buf_len_written = 0;
    int recv_out = -1;
    int errs = 0;
    header_msg_t remote_file_header;
    int remote_file_fd = -1;
    off_t file_offset = 0;
    ssize_t file_size_offset = -1;
    
    //Usage sanity
    if (argc < 2) {
	print_usage();
	exit(-1);
    }
    else {
	listening_port = atoi(argv[1]);
	if ((listening_port <= 1024) || (listening_port >= 65536)) {
	    fprintf(stderr, "local-port must be within the range (1024, 65536)\n");
	    exit(-1);
	}
	else {
	    sprintf(port_num_str, "%d", listening_port);
	    if (0 != gethostname(node_name, LOCAL_HOST_STR_MAX_LEN)) {
		fprintf(stderr, "Error in gethostname:%s\n", strerror(errno));
		exit(-1);
	    }
	}
    }
    
    //get own hostname and address structure related to it
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE; //Free to choose any of the local interfaces
    hints.ai_family = AF_INET;   //Need IPV4
    hints.ai_socktype = SOCK_STREAM; //Need TCP sockets

    if (0 != getaddrinfo(NULL, port_num_str, &hints, &res)) {
	fprintf(stderr, "Error in getaddrinfo: %s\n", strerror(errno));
	exit(-1);
    }

    //Go through valid address list and pick out first match
    //The following iteration isn't really necessary
    for (res_ptr_iter = res; res_ptr_iter != NULL; 
	 res_ptr_iter = res_ptr_iter->ai_next) {
	if ((res_ptr_iter->ai_family == AF_INET) && 
	    (res_ptr_iter->ai_socktype == SOCK_STREAM)) {
	    ip_sock_addr = (struct sockaddr_in *) res_ptr_iter->ai_addr;
	    if (NULL == inet_ntop(res_ptr_iter->ai_family, &(ip_sock_addr->sin_addr), 
				  ip_addr_str, sizeof(ip_addr_str))) {
		fprintf(stderr, "Error in inet_ntop : %s\n", strerror(errno));
		exit(-1);
	    }
	    fprintf(stderr, "Obtained address for %s = %s\n", node_name, ip_addr_str);
	    break;
	}
    }

    //Server starts creates a socket to listen, binds to a port and accepts connections
    listening_socket = socket(res_ptr_iter->ai_family, res_ptr_iter->ai_socktype, 
			      res_ptr_iter->ai_protocol);
    if (-1 == listening_socket) {
	fprintf(stderr, "Error in socket creation:%s\n", strerror(errno));
	exit(-1);
    }

    if (-1 == bind(listening_socket, res_ptr_iter->ai_addr, 
		   res_ptr_iter->ai_addrlen)) {
	fprintf(stderr, "Error in bind:%s\n", strerror(errno));
	exit(-1);
    }

    if (-1 == listen(listening_socket, backlog)) {
	fprintf(stderr, "Error in listen:%s\n", strerror(errno));
	exit(-1);
    }

    remote_socket = accept(listening_socket, (struct sockaddr *)&remote_addr, 
			   &remote_addr_len);
    if (-1 == remote_socket) {
	fprintf(stderr, "Error in accept:%s\n", strerror(errno));
	exit(-1);
    }
    else {
	ip_sock_addr = (struct sockaddr_in *) &remote_addr;
	if (NULL == inet_ntop(res_ptr_iter->ai_family, &(ip_sock_addr->sin_addr), 
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

    //Exchange header information
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
    
    //Initialize some data and then recv from remote and verify
    total_buf_len = remote_file_header.file_size;
    buf_seg_sz = RECV_BUF_SZ / 4;
    
    //for (buf_len_recvd = 0; buf_len_recvd < total_buf_len; buf_len_recvd++) {
    //	recv_buf[buf_len_recvd] = 'b';
    //}
    
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
    
    //for (buf_len_recvd = 0; buf_len_recvd < total_buf_len; buf_len_recvd++) {
    //	if (recv_buf[buf_len_recvd] != 'a') {
    //	    errs++;
    //	}
    //}
    //fprintf(stderr, "num errs = %d\n", errs);

    //Free resources
    if (-1 != remote_file_fd) {
	close(remote_file_fd);
    }
    if (-1 != listening_socket) {
	close(listening_socket);
    }
    freeaddrinfo(res);
    
    return 0;
}
