#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include "general.hh"

#define MAXPORT 65535

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug, std::string& servpath, std::string& username)
{
	bool portgiven = false;
	bool servpathgiven = false;
	bool usernamegiven = false;
	unsigned long candidate;
	char opt;
	while ((opt = getopt(argc, argv, "p:ds:u:")) != -1)
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
			port = (unsigned short)candidate;
			portgiven = true;
			break;
		case 'd':
			debug = true;
			break;
		case 's':
			servpath = std::string(optarg);
			servpathgiven = true;
			break;
		case 'u':
			username = std::string(optarg);
			usernamegiven = true;
			break;
		case '?':
			break;
		default:
			break;
		}
	}
	if (!portgiven || !servpathgiven || !usernamegiven)
	{
		std::cerr << "usage: ./httpserver -p port [-d] -s servpath -u username" << std::endl;
		return -1;
	}
	return 0;
}
