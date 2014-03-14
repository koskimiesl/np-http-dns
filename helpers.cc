#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unistd.h>

#include "helpers.hh"

#define MAXPORT 65535

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug)
{
	bool portgiven = false;
	unsigned long candidate;
	char opt;
	while ((opt = getopt(argc, argv, "p:d")) != -1)
	{
		switch (opt)
		{
		case 'p':
			candidate = std::strtoul(optarg, NULL, 0);
			if (candidate == 0)
			{
				std::cerr << "strtoul: conversion failed" << std::endl;
				break;
			}
			if (candidate > MAXPORT)
			{
				std::cerr << "error: max port is " << MAXPORT << std::endl;
				break;
			}
			port = (unsigned short) candidate;
			std::cout << "using port " << port << std::endl;
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
