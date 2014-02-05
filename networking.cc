#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include "networking.hh"


/* print address used */
void print_address(const char *prefix, const struct addrinfo *res)
{
	char outbuf[80];
	struct sockaddr_in *sin = (struct sockaddr_in *)res->ai_addr;
	struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)res->ai_addr;
	void *address;

	if (res->ai_family == AF_INET)
		address = &(sin->sin_addr);
	else if (res->ai_family == AF_INET6)
		address = &(sin6->sin6_addr);
	else
	{
		std::cerr << "Unknown address" << std::endl;
		return;
	}

	const char *ret = inet_ntop(res->ai_family, address, outbuf, sizeof(outbuf));
	std::cout << prefix << " " << ret << std::endl;
}

int tcp_connect(const char *hostname, const char *service)
{
	int	sockfd, addrret;
	struct addrinfo	hints, *res, *ressave;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((addrret = getaddrinfo(hostname, service, &hints, &res)) != 0)
	{
		std::cerr << "Failed to get address info for " << hostname << ", "
				<< service << ": " << gai_strerror(addrret) << std::endl;
		return -1;
	}
	ressave = res;

	do
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
		{
			perror("Failed to create socket");
			continue; /* ignore this one */
		}

		print_address("Trying to connect", res);
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break; /* success */
		perror("Failed to connect");
		close(sockfd);	/* ignore this one */
	}
	while ((res = res->ai_next) != NULL);

	if (res == NULL)
	{
		std::cerr << "Failed to connect " << hostname << ", " << service << std::endl,
		sockfd = -1;
	}
	else
		print_address("Using address", res);

	freeaddrinfo(ressave);

	return sockfd;
}
