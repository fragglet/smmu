EXE = swantbls.exe

OBJS =                    \
	swantbls.o

all : $(EXE)

$(EXE) : $(OBJS)
	gcc $(OBJS) -o $@

%.o : %.c
	gcc -c $< -o $@
