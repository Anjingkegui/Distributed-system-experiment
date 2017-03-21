CC = g++ -std=c++11 -g 

bin = client server test

all: $(bin)


client:  client.o mqueue.o
	$(CC) $^ -o $@

server: server.o mqueue.o
	$(CC) $^ -o $@

test:  test.o lock.o mqueue.o
	$(CC) $^ -o $@


.cpp.o:
	$(CC) -c $<

.PHONY: clean

clean:
	$(RM) *.o $(bin)
