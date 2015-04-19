/*
  Client code which sends a file to a remote server
*/

#include "common.h"
//#include "tcpd.h"

void print_usage()
{
    fprintf(stderr, "./ftpc remote-host remote-port local-file-to-transfer\n");
}

int main(int argc, char **argv)
{
    int sending_port = -1;
    int sending_socket = -1;
    char node_name[REMOTE_HOST_STR_MAX_LEN];
    char port_num_str[PORT_NUM_STR_MAX_LEN];
    char local_file_name[LOCAL_FILE_NAME_MAX_LEN];
    char ip_addr_str[128];
    struct sockaddr_in *ip_sock_addr;
    struct addrinfo *res, *res_ptr_iter, hints;
    char send_buf[SEND_BUF_SZ];
    int buf_seg_sz = SEND_BUF_SZ / 4;
    int total_buf_len = 0;
    int buf_len_sent = 0;
    int buf_len_read = 0;
    int send_out = -1;
    header_msg_t my_file_header;
    int my_file_fd = -1;
    off_t file_offset = 0;
    ssize_t file_size_offset = -1;
    struct stat my_file_stats;
    
    //Usage sanity
    if (argc < 4) {
	print_usage();
	exit(-1);
    }
    else {
	sending_port = atoi(argv[2]);
	if ((sending_port <= 1024) || (sending_port >= 65536)) {
	    fprintf(stderr, "local-port must be within the range (1024, 65536)\n");
	}
	else {
	    sprintf(node_name, "%s", argv[1]);
	    sprintf(port_num_str, "%d", sending_port);
	    sprintf(local_file_name, "%s", argv[3]);
	    sprintf(my_file_header.file_name, "%s", argv[3]);
	    
	    //Open an existing file for reading alone
	    my_file_fd = open(my_file_header.file_name, O_RDONLY);
	    if (-1 == my_file_fd) {
		fprintf(stderr, "Error in open: %s\n", strerror(errno));
		exit(-1);
	    }
	    //Find the file size
	    //file_offset = lseek(my_file_fd, (off_t) 0, SEEK_END);
	    //if ((off_t) -1 == file_offset) {
	    //	fprintf(stderr, "Error in lseek: %s\n", strerror(errno));
	    //	exit(-1);
	    //}
	    if (-1 == stat(my_file_header.file_name, &my_file_stats)) {
		fprintf(stderr, "Error in stat: %s\n", strerror(errno));
		exit(-1);
	    }
	    my_file_header.file_size = (int) my_file_stats.st_size;
	    ////Reset the offset to head
	    //file_offset = lseek(my_file_fd, (off_t) 0, SEEK_SET);
	    //if ((off_t) -1 == file_offset) {
	    //	fprintf(stderr, "Error in lseek: %s\n", strerror(errno));
	    //	exit(-1);
	    //}
	    fprintf(stderr, "File %s opened (size = %d)\n", my_file_header.file_name, 
		    my_file_header.file_size);
	}
    }
    
    //get own hostname and address structure related to it
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;   //Need IPV4
    hints.ai_socktype = SOCK_STREAM; //Need TCP sockets

    if (0 != getaddrinfo(node_name, port_num_str, &hints, &res)) {
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
	    }
	    fprintf(stderr, "Obtained address for %s = %s\n", node_name, ip_addr_str);
	    break;
	}
    }


    //Client creates a socket and attempts to connect
    sending_socket = socket(res_ptr_iter->ai_family, res_ptr_iter->ai_socktype, 
			      res_ptr_iter->ai_protocol);
    if (-1 == sending_socket) {
	fprintf(stderr, "Error in socket creation:%s\n", strerror(errno));
	exit(-1);
    }

    /*
    if (-1 == bind(sending_socket, res_ptr_iter->ai_addr, 
		   res_ptr_iter->ai_addrlen)) {
	fprintf(stderr, "Error in bind:%s\n", strerror(errno));
	exit(-1);
    }
    */
    
    if (-1 == connect(sending_socket, res_ptr_iter->ai_addr, 
		      res_ptr_iter->ai_addrlen)) {
	fprintf(stderr, "Error in connect:%s\n", strerror(errno));
	exit(-1);
    }
    else {
	fprintf(stderr, "Connected to %s\n", node_name);
    }

    //Exchange header information
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
    
    //Initialize some data and then send it

    buf_seg_sz = SEND_BUF_SZ / 4;
    total_buf_len = my_file_header.file_size;

    //for (buf_len_sent = 0; buf_len_sent < total_buf_len; buf_len_sent++) {
    //	send_buf[buf_len_sent] = 'a';
    //}
    
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
    
    //Free resources
    if (-1 != my_file_fd) {
	close(my_file_fd);
    }
    if (-1 != sending_socket) {
	close(sending_socket);
    }
    freeaddrinfo(res);
    
    return 0;
}
