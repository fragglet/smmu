EXE = smmuserv.exe

CFLAGS = 
LDFLAGS = -lsocket

OBJS =                    \
	smmuserv.o

all : $(EXE)

$(EXE) : $(OBJS)
	gcc $(OBJS) -o $@ $(LDFLAGS)

%.o : %.c
	gcc $(CFLAGS) -c $< -o $@
