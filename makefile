CC = gcc
CFLAGS = -Wall

all: libmsocket.a initmsocket user1 user2 user3 user4
	./initmsocket

libmsocket.a: msocket.o
	ar rcs libmsocket.a msocket.o

initmsocket: initmsocket.o
	$(CC) $(CFLAGS) -o initmsocket initmsocket.o -lpthread

user1: user1.c libmsocket.a
	$(CC) $(CFLAGS) -o user1 user1.c -L. -lmsocket -lpthread

user2: user2.c libmsocket.a
	$(CC) $(CFLAGS) -o user2 user2.c -L. -lmsocket -lpthread

user3: user3.c libmsocket.a
	$(CC) $(CFLAGS) -o user3 user3.c -L. -lmsocket -lpthread

user4: user4.c libmsocket.a
	$(CC) $(CFLAGS) -o user4 user4.c -L. -lmsocket -lpthread

msocket.o: msocket.c msocket.h struct.h
	$(CC) $(CFLAGS) -c msocket.c

initmsocket.o: initmsocket.c struct.h
	$(CC) $(CFLAGS) -c initmsocket.c

clean:
	rm -f *.o *.a *.gch initmsocket user1 user2