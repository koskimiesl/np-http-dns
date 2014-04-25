#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
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
					std::string& filename, std::string& username, std::string& dirpath, std::string& queryname)
{
	bool hostnamegiven = false;
	bool portgiven = false;
	bool methodgiven = false;
	bool filenamegiven = false;
	bool usernamegiven = false;
	bool dirpathgiven = false;
	bool querynamegiven = false;
	char opt;
	while ((opt = getopt(argc, argv, "h:p:m:f:u:d:q:")) != -1)
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

			/* put slash in front of URI/filename if it doesn't exist */
			if (filename.at(0) != '/')
				filename = "/" + filename;

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
		case 'q':
			queryname = std::string(optarg);
			querynamegiven = true;
			break;
		case '?':
			break;
		default:
			break;
		}
	}

	std::transform(method.begin(), method.end(), method.begin(), ::toupper); // method to upper case
	if (method == "GET" || method == "PUT")
	{
		if (!hostnamegiven || !portgiven || !methodgiven || !filenamegiven || !usernamegiven || !dirpathgiven)
		{
			std::cerr << "usage for GET and PUT: ./httpclient -h hostname -p port -m method -f filename -u username -d dirpath" << std::endl;
			return -1;
		}
	}
	else if (method == "POST")
	{
		if (!hostnamegiven || !portgiven || !methodgiven || !querynamegiven || !usernamegiven)
		{
			std::cerr << "usage for POST: ./httpclient -h hostname -p port -m method -q queryname -u username" << std::endl;
			return -1;
		}
	}
	else
	{
		std::cerr << "supported methods: GET, PUT, POST" << std::endl;
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

std::vector<std::string> split_string(const std::string& str, char delimiter)
{
    std::stringstream ss(str);
    std::string item;
    std::vector<std::string> elements;
    while (std::getline(ss, item, delimiter))
    	elements.push_back(item);

    return elements;
}

std::string to_upper(const std::string& str)
{
	std::string uppstr = str;
	std::transform(uppstr.begin(), uppstr.end(), uppstr.begin(), ::toupper);
	return uppstr;
}

general_exception::general_exception(const std::string message) : std::runtime_error(message)
{ }
