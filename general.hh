/* General helper functions */

#ifndef NETPROG_HELPERS_HH
#define NETPROG_HELPERS_HH

#include <stdexcept>
#include <string>
#include <vector>

/* file statuses */
typedef enum
{
	DOES_NOT_EXIST,
	ACCESS_FAILURE,
	OK
} file_status;

/* file permissions */
typedef enum
{
	READ,
	WRITE
} file_permissions;

/*
 * Check file status
 *
 * path: path to file
 * perm: permissions to check
 * return: status enum value
 */
file_status check_file_status(std::string path, file_permissions perm);

/*
 * Check file size
 *
 * path: path to file
 * return: filesize or -1 on error
 */
int check_file_size(std::string path);

/*
 * Create directory if it does not exist
 *
 * path: directory path
 * return: 0 on success, -1 on error
 */
int create_dir(std::string path);

/*
 * Get client command line options
 *
 * argc: number of args
 * argv: arguments
 * hostname: server hostname
 * port: server port
 * method: method to use
 * filename: filename for the request
 * username: iam header field
 * dirpath: directory for files
 * queryname: name to be queried from DNS
 * return: 0 on success, -1 on error
 */
int get_client_opts(int argc, char** argv, std::string& hostname, std::string& port, std::string& method,
					std::string& filename, std::string& username, std::string& dirpath, std::string& queryname);

/*
 * Get server command line options
 *
 * argc: number of arguments
 * argv: arguments
 * port: port to listen
 * debug: daemonize or not
 * servpath: path to serving directory
 * dnsservip: IP of DNS server to use
 * username: iam header field
 * return: 0 on success, -1 on error
 */
int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug,
					std::string& servpath, std::string& dnsservip, std::string& username);

/*
 * Split a string into tokens
 *
 * str: string to split
 * delimiter: delimiter to use in splitting
 * return: vector of tokens
 */
std::vector<std::string> split_string(const std::string& str, char delimiter);

/*
 * Transform a string into upper case
 *
 * str: string to transfrom
 * return: string in upper case
 */
std::string to_upper(const std::string& str);

/* general exception to be used */
class general_exception : public std::runtime_error
{
public:

	general_exception(const std::string message);
};

#endif
