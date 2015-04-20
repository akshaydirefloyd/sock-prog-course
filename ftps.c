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
    char node_name[LOCAL_HOST_STR_MAX_LEN];
    char port_num_str[PORT_NUM_STR_MAX_LEN];
    char remote_node_name[REMOTE_HOST_STR_MAX_LEN];
    struct addrinfo *server_addr;
    int remote_socket;
    header_msg_t remote_file_header;
    int remote_file_fd = -1;
    
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

    //initialize some basic things
    init_all();
    
    //get own hostname and address structure related to it
    setup_addr(SOCK_TYPE_STREAM, NULL, port_num_str, &server_addr);

    //Server starts creates a socket to listen, binds to a port and accepts connections
    setup_server_cxn(server_addr, &remote_socket, remote_node_name);
    fprintf(stderr, "ftpc node name = %s\n", remote_node_name);

    //Exchange header information
    remote_file_header = recv_header(remote_socket);

    // Create file for the remote client
    remote_file_fd = create_client_file(remote_node_name, remote_file_header);

    // Get remote file content
    recv_file(remote_socket, remote_file_fd, remote_file_header);

    //Free resources
    if (-1 != remote_file_fd) {
	close(remote_file_fd);
    }

    free_sockets();
    free_addresses();
    
    return 0;
}
