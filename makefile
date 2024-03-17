CC = gcc
CFLAGS = -Wall

libmsocket.a: msocket.o
	ar rcs libmsocket.a msocket.o

initmsocket: initmsocket.o
	$(CC) $(CFLAGS) -o initmsocket initmsocket.o -lpthread

user1: user1.c libmsocket.a
	$(CC) $(CFLAGS) -o user1 user1.c -lmsocket -L.

user2: user2.c libmsocket.a
	$(CC) $(CFLAGS) -o user2 user2.c -lmsocket -L.

msocket.o: msocket.c msocket.h
	$(CC) $(CFLAGS) -c msocket.c

initmsocket.o: initmsocket.c
	$(CC) $(CFLAGS) -c initmsocket.c

clean:
	rm -f *.o *.a *.gch initmsocket user1 user2