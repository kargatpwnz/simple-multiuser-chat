CC := g++
CFLAGS := -Wall -std=c++11
LDFLAGS := -pthread

SERVER_EXEC := server_main
CLIENT_EXEC := client_main

server := server_main
server_obj := server_main.o Server.o Client.o

client := client_main
client_obj := client_main.o Client.o

LIB_DIR := src

.PHONY: all clean

all: $(client) $(server)

$(server): $(server_obj)
	$(CC) -o $@ $^ $(LDFLAGS) 

$(client): $(client_obj)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

Server.o: Server.cpp 
Client.o: Client.cpp

clean:
	rm -rf *.o $(server) $(client)
