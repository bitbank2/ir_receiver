CFLAGS=-c -Wall -O3
LINKFLAGS=-lm -lpthread -larmbianio

all: ir

ir: main.o
	$(CC) main.o $(LINKFLAGS) -o ir

main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	rm *.o ir

