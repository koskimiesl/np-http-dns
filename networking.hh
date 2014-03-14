/* Networking helper functions */

#ifndef NETPROG_NETWORKING_HH
#define NETPROG_NETWORKING_HH

#include <string>

/* create TCP socket, bind server address to it and listen
 * return socket file descriptor */
int create_and_listen(unsigned short port);

/* send message to socket
 * return bytes sent */
int send_message(int sockfd, std::string message);

/* send message to socket
 * return bytes sent */
int send_message(int sockfd, std::string header, const char* payload, size_t pllength);

/* create and connect TCP socket
 * return socket file descriptor */
int tcp_connect(const char *hostname, const char *service);

#endif
