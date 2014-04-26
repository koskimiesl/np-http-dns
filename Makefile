CPP = g++
FLAGS = -std=c++0x -Wall -Wextra -pedantic -lpthread

objects_server = server.o daemon.o dns.o general.o http.o httpconf.o networking.o threading.o
objects_client = client.o dns.o general.o http.o httpconf.o networking.o

objects = server.o client.o daemon.o dns.o general.o http.o httpconf.o networking.o threading.o

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

dns.o: dns.cc
	$(CPP) -c $^ $(FLAGS)

general.o: general.cc
	$(CPP) -c $^ $(FLAGS)

http.o: http.cc
	$(CPP) -c $^ $(FLAGS)
	
httpconf.o: httpconf.cc
	$(CPP) -c $^ $(FLAGS)

networking.o: networking.cc
	$(CPP) -c $^ $(FLAGS)
	
threading.o: threading.cc
	$(CPP) -c $^ $(FLAGS)

# header dependencies
server.o: daemon.hh general.hh http.hh networking.hh threading.hh
client.o: general.hh http.hh networking.hh
daemon.o: daemon.hh
dns.o: dns.hh networking.hh
general.o: general.hh
http.o: dns.hh general.hh http.hh networking.hh
httpconf.o: httpconf.hh
networking.o: networking.hh
threading.o: threading.hh

.PHONY: clean
clean:
	rm httpserver httpclient $(objects) *.gch
