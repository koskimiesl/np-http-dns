#include <cstring>
#include <iostream>
#include <unistd.h>

#include "helpers.hh"

int get_server_opts(int argc, char** argv, char* port, bool& debug)
{
	bool portgiven = false;
	char opt;
	while ((opt = getopt(argc, argv, "p:d")) != -1)
	{
		switch (opt)
		{
		case 'p':
			strncpy(port, optarg, PORTLEN);
			portgiven = true;
			break;
		case 'd':
			debug = true;
			break;
		case '?':
			break;
		default:
			break;
		}
	}
	if (!portgiven)
	{
		std::cerr << "usage: ./httpserver -p port [-d]" << std::endl;
		return -1;
	}
	return 0;
}
