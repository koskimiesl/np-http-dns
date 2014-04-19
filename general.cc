#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

#include "general.hh"

#define MAXPORT 65535

file_status check_file(std::string path, file_permissions perm)
{
	/* check file existence */
	if (access(path.c_str(), F_OK) < 0)
	{
		perror("access");
		return file_status::DOES_NOT_EXIST;
	}

	/* check file permissions */
	switch (perm)
	{
	case file_permissions::READ:
		if (access(path.c_str(), R_OK) < 0)
		{
			perror("access");
			return file_status::ACCESS_FAILURE;
		}
		break;
	case file_permissions::WRITE:
		if (access(path.c_str(), W_OK) < 0)
		{
			perror("access");
			return file_status::ACCESS_FAILURE;
		}
		break;
	default:
		return file_status::ACCESS_FAILURE;
	}

	return file_status::OK;
}

int create_dir(std::string path)
{
	struct stat st;
	if (stat(path.c_str(), &st) == 0) // path exists
	{
		std::cout << "directory '" << path << "' exists" << std::endl;
		return 0;
	}
	if (mkdir(path.c_str(), 0777) < 0)
	{
		perror("mkdir");
		return -1;
	}
	std::cout << "created directory '" << path << "'" << std::endl;
	return 0;
}

int get_client_opts(int argc, char** argv, std::string& hostname, std::string& port, std::string& method,
					std::string& filename, std::string& username, std::string& dirpath)
{
	bool hostnamegiven = false;
	bool portgiven = false;
	bool methodgiven = false;
	bool filenamegiven = false;
	bool usernamegiven = false;
	bool dirpathgiven = false;
	char opt;
	while ((opt = getopt(argc, argv, "h:p:m:f:u:d:")) != -1)
	{
		switch (opt)
		{
		case 'h':
			hostname = std::string(optarg);
			hostnamegiven = true;
			break;
		case 'p':
			port = std::string(optarg);
			portgiven = true;
			break;
		case 'm':
			method = std::string(optarg);
			methodgiven = true;
			break;
		case 'f':
			filename = std::string(optarg);
			filenamegiven = true;
			break;
		case 'u':
			username = std::string(optarg);
			usernamegiven = true;
			break;
		case 'd':
			dirpath = std::string(optarg);
			dirpathgiven = true;
			break;
		case '?':
			break;
		default:
			break;
		}
	}
	if (!hostnamegiven || !portgiven || !methodgiven || !filenamegiven || !usernamegiven || !dirpathgiven)
	{
		std::cerr << "usage: ./httpclient -h hostname -p port -m method -f filename -u username -d dirpath" << std::endl;
		return -1;
	}
	return 0;
}

int get_file_size(std::string path)
{
	std::ifstream fs(path);
	if (!fs.good()) // check stream state
	{
		std::cerr << "file stream error" << std::endl;
		return -1;
	}
	fs.seekg(0, fs.end);
	int filesize = fs.tellg();
	fs.close();

	return filesize;
}

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug,
					std::string& servpath, std::string& username)
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
