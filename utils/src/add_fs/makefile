# default:
EXE = add_fs.exe

OBJS =             \
	main.o     \
	waddir.o

all : $(EXE)

$(EXE) : $(OBJS)
	gcc $(OBJS) -o $@

%.o : %.c
	gcc -c $< -o $@

main.o : waddir.h
waddir.o : waddir.h
