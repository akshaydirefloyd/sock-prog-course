CC = gcc
FTPC = ftpc.c common.c
FTPS = ftps.c common.c
#TCPDC = tcpdc.c
#TCPDS = tcpds.c
CFLAGS = -Wall
# setup for system
LIBS =

all: ftpc.out ftps.out # tcpdc tcpds

ftpc.out:	$(FTPC)
		$(CC) $(CFLAGS) -o $@ $(FTPC) $(LIBS)
ftps.out:	$(FTPS)
		$(CC) $(CFLAGS) -o $@ $(FTPS) $(LIBS)

#tcpds:	$(TCPDS)
#	$(CC) $(CFLAGS) -o $@ $(TCPDS) $(LIBS)
#tcpdc:	$(TCPDC)
#	$(CC) $(CFLAGS) -o $@ $(TCPDC) $(LIBS)
clean:
	rm ftpc.out ftps.out #tcpdc tcpds
