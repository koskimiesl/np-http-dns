CPP = g++
FLAGS = -std=c++0x -Wall -Wextra -pedantic -lpthread

objects_server = server.o daemon.o general.o http.o networking.o threading.o
objects_client = client.o general.o http.o networking.o

objects = server.o client.o daemon.o general.o http.o networking.o threading.o

PROGS = server client

all: $(PROGS)

server: $(objects_server)
	$(CPP) -o httpserver $(objects_server) $(FLAGS)

client: $(objects_client)
	$(CPP) -o httpclient $(objects_client) $(FLAGS)

server.o: server.cc
	$(CPP) -c $^ $(FLAGS)

client.o: client.cc
	$(CPP) -c $^ $(FLAGS)

daemon.o: daemon.cc
	$(CPP) -c $^ $(FLAGS)

general.o: general.cc
	$(CPP) -c $^ $(FLAGS)

http.o: http.cc
	$(CPP) -c $^ $(FLAGS)

networking.o: networking.cc
	$(CPP) -c $^ $(FLAGS)
	
threading.o: threading.cc
	$(CPP) -c $^ $(FLAGS)

# header dependencies
server.o: daemon.hh general.hh http.hh networking.hh threading.hh
client.o: general.hh http.hh networking.hh
daemon.o: daemon.hh
general.o: general.hh
http.o: general.hh http.hh networking.hh
networking.o: networking.hh
threading.o: threading.hh

.PHONY: clean
clean:
	rm httpserver httpclient $(objects) *.gch
