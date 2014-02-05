CPP = g++
FLAGS = -std=c++0x -Wall -Wextra

objects_client = client.o networking.o

objects = client.o networking.o

PROGS = client

all: $(PROGS)

client: $(objects_client)
	$(CPP) -o client $(objects_client) $(FLAGS)

client.o: client.cc
	$(CPP) -c $^ $(FLAGS)

networking.o: networking.cc
	$(CPP) -c $^ $(FLAGS)

# header dependencies
client.o: networking.hh
networking.o: networking.hh

.PHONY: clean
clean:
	rm client $(objects) *.gch
