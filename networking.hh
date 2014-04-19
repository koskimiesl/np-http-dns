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

bool read_header(int sockfd, std::string delimiter, std::string& header);

bool recv_text_file(int sockfd, std::string dirpath, std::string filename, size_t filesize);

/* Send string message to socket
 * Caller must open and close the socket descriptor
 *
 * param sockfd: socket descriptor
 * param message: message to send
 * param continues: if true, message continues
 * return: true on success, false on failure */
bool send_message(int sockfd, std::string message, bool continues);

/* Send text file to socket
 * Caller must open and close the socket descriptor
 *
 * param sockfd:
 * param servpath:
 * param filename:
 * param filesize:
 * return: true on success, false on failure */
bool send_text_file(int sockfd, std::string servpath, std::string filename, size_t filesize);

/* create and connect TCP socket
 * return socket file descriptor */
int tcp_connect(std::string hostname, std::string port);

#endif
