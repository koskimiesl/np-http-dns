/* Networking helper functions */

#ifndef NETWORKING_HH
#define NETWORKING_HH

#include <string>

/* send message to socket
 * return bytes sent */
int send_message(int sockfd, std::string message);

/* create and connect TCP socket
 * return file descriptor */
int tcp_connect(const char *hostname, const char *service);

#endif
