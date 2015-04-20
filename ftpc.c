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
    struct addrinfo *server_addr;
    header_msg_t my_file_header;
    int my_file_fd = -1;
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

    //initialize some basic things
    init_all();
    
    //get server hostname and address structure related to it
    setup_addr(SOCK_TYPE_STREAM, node_name, port_num_str, &server_addr);

    //Client creates a socket and attempts to connect
    sending_socket = setup_client_cxn(server_addr);

    //Exchange header information
    send_header(sending_socket, my_file_header);

    //Send file
    send_file(sending_socket, my_file_header, my_file_fd);
    
    //Free resources
    if (-1 != my_file_fd) {
	close(my_file_fd);
    }

    free_sockets();
    free_addresses();
    
    return 0;
}
