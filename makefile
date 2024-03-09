CC = gcc
CFLAGS = -Wall

libmsocket.a: msocket.o
	ar rcs libmsocket.a msocket.o

initmsocket: initmsocket.o
	$(CC) $(CFLAGS) -o initmsocket initmsocket.o -lpthread

user1: user1.o libmsocket.a
	$(CC) $(CFLAGS) -o user1 user1.o -lmsocket

user2: user2.o libmsocket.a
	$(CC) $(CFLAGS) -o user2 user2.o -lmsocket

msocket.o: msocket.c msocket.h
	$(CC) $(CFLAGS) -c msocket.c

initmsocket.o: initmsocket.c
	$(CC) $(CFLAGS) -c initmsocket.c

user1.o: user1.c msocket.h
	$(CC) $(CFLAGS) -c user1.c

user2.o: user2.c msocket.h
	$(CC) $(CFLAGS) -c user2.c

clean:
	rm -f *.o *.a initmsocket user1 user2