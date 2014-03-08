CPP = g++
FLAGS = -std=c++0x -Wall -Wextra -pedantic

objects_server = server.o daemon.o
objects_client = client.o networking.o http.o

objects = server.o client.o networking.o http.o daemon.o

PROGS = server client

all: $(PROGS)

server: $(objects_server)
	$(CPP) -o httpserver $(objects_server) $(FLAGS)

client: $(objects_client)
	$(CPP) -o client $(objects_client) $(FLAGS)

server.o: server.cc
	$(CPP) -c $^ $(FLAGS)

client.o: client.cc
	$(CPP) -c $^ $(FLAGS)

daemon.o: daemon.cc
	$(CPP) -c $^ $(FLAGS)
	
http.o: http.cc
	$(CPP) -c $^ $(FLAGS)

networking.o: networking.cc
	$(CPP) -c $^ $(FLAGS)

# header dependencies
server.o: daemon.hh
client.o: networking.hh http.hh
http.o: http.hh
networking.o: networking.hh

.PHONY: clean
clean:
	rm httpserver client $(objects) *.gch
