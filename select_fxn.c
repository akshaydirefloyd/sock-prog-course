#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#define STDIN 0

int main (int argc, char **argv)
{
    struct timeval tv;
    fd_set readfds;
    char msg[1024];

    tv.tv_sec = 20;
    tv.tv_usec = 2;


    while(1) {
    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);
	select(STDIN + 2, &readfds, NULL, NULL, &tv);

	if (FD_ISSET(STDIN, &readfds)) {
	    fprintf(stderr, "key pressed\n");
	    scanf("%s", msg);
	    if (0 == strcmp(msg, "x")) {
		fprintf(stderr, "x pressed\n");
		break;
	    }
	}
	else {
	    fprintf(stderr, "Time out\n");
	}
	fprintf(stderr, "%s\n", msg);
    }
    return 0;
}
