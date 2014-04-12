CPP = g++
FLAGS = -std=c++0x -Wall -Wextra -pedantic -lpthread

objects_server = server.o daemon.o helpers.o http.o networking.o
objects_client = client.o networking.o http.o

objects = server.o client.o networking.o http.o daemon.o helpers.o

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
	
helpers.o: helpers.cc
	$(CPP) -c $^ $(FLAGS)

# header dependencies
server.o: daemon.hh helpers.hh http.hh networking.hh
client.o: networking.hh http.hh
http.o: http.hh
networking.o: networking.hh
daemon.o: daemon.hh
helpers.o: helpers.hh

.PHONY: clean
clean:
	rm httpserver client $(objects) *.gch
