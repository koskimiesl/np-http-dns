/* Networking helper functions */

#ifndef NETWORKING_HH
#define NETWORKING_HH


/* create and connect TCP socket
 * return file descriptor */
int tcp_connect(const char *hostname, const char *service);

#endif
