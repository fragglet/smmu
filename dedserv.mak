#=========================================================================
#
# Makefile for SMMU dedicated server
#
#=========================================================================

CC = gcc
CFLAGS = -Wall -g -O2 -DTCPIP -DLINUX -DDEDICATED
LDFLAGS =


OBJS =                 \
	sv_serv.ded.o      \
	net_udp.ded.o      \
	net_gen.ded.o      \
	dedserv.ded.o

all: dedserv

dedserv: $(OBJS) version.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) version.o -o $@

%.ded.o:   %.c
	$(CC) $(CFLAGS) -c $< -o $@

version.o: $(OBJS)