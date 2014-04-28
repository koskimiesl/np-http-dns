/* Networking helper functions */

#ifndef NETPROG_NETWORKING_HH
#define NETPROG_NETWORKING_HH

#include <string>
#include <sys/socket.h>

/*
 * Accept connection and set 5 second receive timeout for the connection
 *
 * listenfd: socket descriptor set to listen mode
 * return: new socket descriptor with timeout set
 */
int accept_connection(int listenfd);

/*
 * Create TCP socket, bind server address to it and listen
 *
 * port: server port
 * return: socket descriptor or -1 on error
 */
int create_and_listen(unsigned short port);

/*
 * Init UDP socket and set 5 second receive timeout for it
 *
 * destip: destination address
 * destport: destination port
 * destaddr: pointer to save destination address pointer
 * addrlen: pointer to save address length
 * return: socket descriptor or -1 on error
 */
int init_udp(const char* destip, const char* destport, struct sockaddr** destaddr, socklen_t* addrlen);

/*
 * Read header from socket
 *
 * sockfd: socket descriptor
 * delimiter: delimiter to separate header and payload
 * header: result of read
 * return: true on success, false on failure
 */
bool read_header(int sockfd, std::string delimiter, std::string& header);

/*
 * Receive body from socket
 *
 * sockfd: socket descriptor
 * contentlen: body length
 * body: body received
 * return: true on success, false on failure
 */
bool recv_body(int sockfd, size_t contentlen, std::string& body);

/*
 * Receive text file from socket
 *
 * sockfd: socket descriptor
 * dirpath: path to serving directory
 * filename: file to receive
 * filesize: size of file in bytes
 * return: true on success, false on failure
 */
bool recv_text_file(int sockfd, std::string dirpath, std::string filename, size_t filesize);

/*
 * Send string message to socket
 *
 * sockfd: socket descriptor
 * message: message to send
 * uselength: if true, contentlen will be used
 * contentlen: content length
 * continues: if true, message continues
 * return: true on success, false on failure
 */
bool send_message(int sockfd, std::string message, bool uselength, size_t contentlen, bool continues);

/*
 * Send text file to socket
 *
 * sockfd: socket descriptor
 * servpath: path to serving directory
 * filename: file to send
 * filesize: size of file in bytes
 * return: true on success, false on failure
 */
bool send_text_file(int sockfd, std::string servpath, std::string filename, size_t filesize);

/*
 * Create and connect TCP socket
 *
 * hostname: hostname to connect
 * port: port to connect
 * return: socket descriptor or -1 on error
 */
int tcp_connect(std::string hostname, std::string port);

#endif
