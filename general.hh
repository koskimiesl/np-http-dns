/* General helper functions */

#ifndef NETPROG_HELPERS_HH
#define NETPROG_HELPERS_HH

#include <string>

enum file_status
{
	DOES_NOT_EXIST,
	ACCESS_FAILURE,
	OK
};

enum file_permissions
{
	READ,
	WRITE
};

file_status check_file(std::string path, file_permissions perm);

int create_dir(std::string path);

int get_client_opts(int argc, char** argv, std::string& hostname, std::string& port, std::string& method,
					std::string& filename, std::string& username, std::string& dirpath);

int get_file_size(std::string path);

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug,
					std::string& servpath, std::string& username);

#endif
