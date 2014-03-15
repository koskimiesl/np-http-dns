/* General helper functions */

#ifndef NETPROG_HELPERS_HH
#define NETPROG_HELPERS_HH

#include <string>

int get_server_opts(int argc, char** argv, unsigned short& port, bool& debug, std::string& username);

#endif
