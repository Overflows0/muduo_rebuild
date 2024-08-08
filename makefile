all: client server
client:
	g++ -g -I.. *.cpp asio/ChatClient.cpp -lpthread -o ChatClient

server:
	g++ -g -I.. *.cpp asio/ChatServer.cpp -lpthread -o ChatServer

clean:
	rm -f *.o

.PHONY: all client server clean
