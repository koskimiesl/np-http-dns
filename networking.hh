/* Networking helper functions */

#ifndef NETPROG_NETWORKING_HH
#define NETPROG_NETWORKING_HH

#include <string>

/* Accept connection and set 5 second receive timeout for the connection
 *
 * param listenfd: socket descriptor set to listen mode
 * returns: new socket descriptor with timeout set
 */
int accept_connection(int listenfd);

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
int tcp_connect(std::string hostname, std::string port);

#endif
