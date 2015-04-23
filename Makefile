CC = gcc
FTPC = ftpc.c common.c sock_wrapper_api.c tcpd_help.c
FTPS = ftps.c common.c sock_wrapper_api.c tcpd_help.c
TCPD = tcpd.c
CFLAGS = -Wall
# setup for system
LIBS =

all: ftpc.out ftps.out tcpd.out # tcpdc tcpds

ftpc.out:	$(FTPC)
		$(CC) $(CFLAGS) -o $@ $(FTPC) $(LIBS)
ftps.out:	$(FTPS)
		$(CC) $(CFLAGS) -o $@ $(FTPS) $(LIBS)
tcpd.out:	$(TCPD)
		$(CC) $(CFLAGS) -o $@ $(TCPD) $(LIBS)

#tcpds:	$(TCPDS)
#	$(CC) $(CFLAGS) -o $@ $(TCPDS) $(LIBS)
#tcpdc:	$(TCPDC)
#	$(CC) $(CFLAGS) -o $@ $(TCPDC) $(LIBS)
clean:
	rm ftpc.out ftps.out tcpd.out #tcpdc tcpds
