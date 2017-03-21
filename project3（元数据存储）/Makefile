CC = g++ -std=c++11 -g  
LIBS = `pkg-config --cflags --libs protobuf` -lboost_system -lcrypto #`locate boost_system`

bin = meta 

all: $(bin)

meta: main.o miniDFS.o nameserver.o dataserver.o msgdelivery.o message.pb.o socket.o 
	$(CC)  -o $@ $^ $(LIBS) 

upload: upload.o miniDFS.o msgdelivery.o message.pb.o socket.o
	$(CC)  -o $@ $^ $(LIBS) 

read: read.o miniDFS.o msgdelivery.o message.pb.o socket.o
	$(CC)  -o $@ $^ $(LIBS) 

socket: socket.o 
	$(CC)  -o $@ $^ $(LIBS) 

.cc.o:
	$(CC) -c $<

.PHONY: clean

clean:
	$(RM) *.o $(bin)
