/* Networking helper functions */

#ifndef NETWORKING_HH
#define NETWORKING_HH

#include <string>

/* send message to socket
 * return bytes sent */
int send_message(int sockfd, std::string message);

/* send message to socket
 * return bytes sent */
int send_message(int sockfd, std::string header, const char* payload, size_t pllength);

/* create and connect TCP socket
 * return file descriptor */
int tcp_connect(const char *hostname, const char *service);

#endif
