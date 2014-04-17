/* General helper functions */

#ifndef NETPROG_HELPERS_HH
#define NETPROG_HELPERS_HH

#include <string>

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug,
					std::string& servpath, std::string& username);

int get_client_opts(int argc, char** argv, std::string& hostname, std::string& port,
					std::string& method, std::string& filename, std::string& username);

#endif
