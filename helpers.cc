#include <climits>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include "helpers.hh"

#define MAXPORT 65535

int create_dir(const std::string path)
{
	struct stat st;
	if (stat(path.c_str(), &st) == 0) // directory exists
		return 0;
	if (mkdir(path.c_str(), 0777) == -1)
	{
		perror("mkdir");
		return -1;
	}
	return 0;
}

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug, std::string& username)
{
	bool portgiven = false;
	bool usernamegiven = false;
	unsigned long candidate;
	char opt;
	while ((opt = getopt(argc, argv, "p:du:")) != -1)
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
	if (!portgiven || !usernamegiven)
	{
		std::cerr << "usage: ./httpserver -p port [-d] -u username" << std::endl;
		return -1;
	}
	return 0;
}
