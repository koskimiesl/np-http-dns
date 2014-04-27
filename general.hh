/* General helper functions */

#ifndef NETPROG_HELPERS_HH
#define NETPROG_HELPERS_HH

#include <stdexcept>
#include <string>
#include <vector>

typedef enum
{
	DOES_NOT_EXIST,
	ACCESS_FAILURE,
	OK
} file_status;

typedef enum
{
	READ,
	WRITE
} file_permissions;

file_status check_file_status(std::string path, file_permissions perm);

int check_file_size(std::string path);

int create_dir(std::string path);

int get_client_opts(int argc, char** argv, std::string& hostname, std::string& port, std::string& method,
					std::string& filename, std::string& username, std::string& dirpath, std::string& queryname);

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug,
					std::string& servpath, std::string& dnsservip, std::string& username);

std::vector<std::string> split_string(const std::string& str, char delimiter);

std::string to_upper(const std::string& str);

class general_exception : public std::runtime_error
{
public:

	general_exception(const std::string message);
};

#endif
