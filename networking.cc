#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#include "networking.hh"

#define LISTENQLEN 5
#define READBUFSIZE 1024
#define SENDBUFSIZE 512

int accept_connection(int listenfd)
{
	int connfd;
	struct sockaddr_in6	addr;
	socklen_t len = sizeof(addr);
	if ((connfd = accept(listenfd, (struct sockaddr*)&addr, &len)) < 0)
	{
		perror("accept");
		return -1;
	}

	/* set 5 second receive timeout */
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)) < 0)
	{
		perror("setsockopt");
		return -1;
	}

	char buff[80];
	std::cout << "connection from " << inet_ntop(AF_INET6, &addr.sin6_addr, buff, sizeof(buff))
			  << ", port " << ntohs(addr.sin6_port) << ", fd is " << connfd << std::endl;

	return connfd;
}

int create_and_listen(unsigned short port)
{
	int listenfd;

	// create socket
	if ((listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("socket");
		return -1;
	}

	// bind server address to socket
	struct sockaddr_in6	servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin6_family = AF_INET6;
	servaddr.sin6_addr = in6addr_any; // any interface
	servaddr.sin6_port = htons(port);
	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		return -1;
	}

	// set socket to passive mode
	if (listen(listenfd, LISTENQLEN) < 0)
	{
		perror("listen");
		return -1;
	}

	return listenfd;
}

int init_udp(const char* destip, const char* destport, struct sockaddr** destaddr, socklen_t* addrlen)
{
	int	sockfd, n;
	struct addrinfo hints, *res, *ressave;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	if ((n = getaddrinfo(destip, destport, &hints, &res)) != 0)
	{
		fprintf(stderr, "init_udp error for %s, %s: %s\n", destip, destport, gai_strerror(n));
	    return -1;
	}
	ressave = res;

	do
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	    if (sockfd < 0)
	    	continue; /* ignore this one */

		break; // yes, this is pretty dumb loop :-)
	} while ((res = res->ai_next) != NULL);

	freeaddrinfo(ressave);

	if (res == NULL)
	{
		fprintf(stderr, "Could not open socket\n");
		return -1;
	}

	/* set 5 second receive timeout */
	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval)) < 0)
	{
		perror("setsockopt");
		return -1;
	}

	*destaddr = res->ai_addr;
	*addrlen = res->ai_addrlen;

	return sockfd;
}

bool read_header(int sockfd, std::string delimiter, std::string& header)
{
	bool delimiterfound = false;
	int recvd;
	char byte; // 1 byte buffer
	std::string readtotal;
	size_t foundidx; // index of start of delimiter
	while (!delimiterfound && (recvd = read(sockfd, &byte, 1)) == 1)
	{
		readtotal += std::string(&byte, 1);
		foundidx = readtotal.find(delimiter);
		if (foundidx != std::string::npos)
		{
			delimiterfound = true;
			header = readtotal.substr(0, foundidx + delimiter.length()); // include delimiter to header
		}
	}
	if (recvd < 0)
	{
		perror("read");
		return false;
	}
	if (!delimiterfound)
	{
		std::cerr << "delimiter not found" << std::endl;
		return false;
	}
	return true;
}

bool recv_body(int sockfd, size_t contentlen, std::string& body)
{
	std::cout << "receiving body of " << contentlen << " bytes...";
	size_t recvdsofar = 0;
	int recvd;
	char buffer[READBUFSIZE];
	std::string bodyrecvd;
	while (recvdsofar < contentlen && (recvd = read(sockfd, buffer, READBUFSIZE)) > 0)
	{
		std::string chunk(buffer, recvd);
		recvdsofar += recvd;
		bodyrecvd += chunk;
	}
	if (recvd == 0)
	{
		std::cerr << "eof" << std::endl;
		std::cerr << "body received so far: " << bodyrecvd << std::endl;
		return false;
	}
	if (recvd < 0)
	{
		perror("read");
		return false;
	}
	body = bodyrecvd;
	std::cout << recvdsofar << " bytes received" << std::endl;
	return true;
}

bool recv_text_file(int sockfd, std::string dirpath, std::string filename, size_t filesize)
{
	std::cout << "receiving file of " << filesize << " bytes...";
	std::ofstream fs(dirpath + filename);
	if (!fs.good()) // check stream state
	{
		std::cerr << "file stream error" << std::endl;
		return false;
	}

	size_t recvdsofar = 0;
	int recvd;
	char buffer[READBUFSIZE];
	while (recvdsofar < filesize && (recvd = read(sockfd, buffer, READBUFSIZE)) > 0)
	{
		std::string chunk(buffer, recvd);
		recvdsofar += recvd;
		fs << chunk;
	}
	if (recvd < 0)
	{
		perror("read");
		fs.close();
		return false;
	}
	if (recvd == 0)
	{
		std::cerr << "eof" << std::endl;
		fs.close();
		return false;
	}
	fs.close();
	std::cout << recvdsofar << " bytes received" << std::endl;
	return true;
}

bool send_message(int sockfd, std::string message, bool uselength, size_t contentlen, bool continues)
{
	const char* msg = message.c_str();
	std::cout << std::endl << "sending message:" << std::endl << msg << std::endl;
	size_t remaining; // number of bytes remaining to send
	if (uselength) // use the given content length
		remaining = contentlen;
	else
	{
		remaining = strlen(msg);
		if (!continues) // if message doesn't continue, write also terminating null character
			remaining++;
	}

	ssize_t sent;
	size_t byteidx = 0;
	size_t chunktosend = SENDBUFSIZE;
	while (remaining > 0)
	{
		if (remaining < chunktosend)
			chunktosend = remaining;
		if ((sent = write(sockfd, &msg[byteidx], chunktosend)) < 0)
		{
			perror("write");
			return false;
		}
		byteidx += sent;
		remaining -= sent;
		std::cout << sent << " bytes sent, " << remaining << " bytes remaining" << std::endl;
	}
	if (remaining == 0)
		return true;
	std::cerr << "bytes remaining is not zero!" << std::endl;
	return false;
}

bool send_text_file(int sockfd, std::string servpath, std::string filename, size_t filesize)
{
	std::cout << "sending file of " << filesize << " bytes...";
	std::ifstream fs(servpath + filename);
	if (!fs.good()) // check stream state
	{
		std::cerr << "file stream error" << std::endl;
		return false;
	}

	size_t totalsent = 0;

	/* read file in chunks */
	char readbuffer[READBUFSIZE];
	int chunktoread = READBUFSIZE;
	size_t readremaining = filesize;
	while (readremaining > 0)
	{
		if (readremaining < (size_t)chunktoread)
			chunktoread = readremaining;
		fs.read(readbuffer, chunktoread);
		if (!fs.good() && !fs.eof())
		{
			std::cerr << "file stream error (not eof)" << std::endl;
			fs.close();
			return false;
		}
		readremaining -= chunktoread;

		/* send read chunk to socket in chunks */
		size_t sendremaining = chunktoread; // number of bytes remaining to send
		ssize_t sent;
		size_t byteidx = 0;
		size_t chunktosend = SENDBUFSIZE;
		while (sendremaining > 0)
		{
			if (sendremaining < chunktosend)
				chunktosend = sendremaining;
			if ((sent = write(sockfd, &readbuffer[byteidx], chunktosend)) < 0)
			{
				perror("write");
				fs.close();
				return false;
			}
			byteidx += sent;
			sendremaining -= sent;
			totalsent += sent;
		}
		if (sendremaining != 0)
		{
			std::cerr << "send remaining is not zero!" << std::endl;
			fs.close();
			return false;
		}
	}

	fs.close();
	std::cout << totalsent << " bytes sent" << std::endl;
	return true;
}

/*
 * Print address
 */
void print_address(const char *prefix, const struct addrinfo *res)
{
	/* slightly modified from lecture example */

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
		std::cerr << "unknown address" << std::endl;
		return;
	}

	const char *ret = inet_ntop(res->ai_family, address, outbuf, sizeof(outbuf));
	std::cout << prefix << " " << ret << std::endl;
}

int tcp_connect(std::string hostname, std::string port)
{
	/* slightly modified from lecture example */

	int	sockfd, addrret;
	struct addrinfo	hints, *res, *ressave;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((addrret = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &res)) != 0)
	{
		std::cerr << "failed to get address info for " << hostname << ", "
				  << port << ": " << gai_strerror(addrret) << std::endl;
		return -1;
	}
	ressave = res;

	do
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd < 0)
		{
			perror("failed to create socket");
			continue; // ignore this one
		}

		print_address("trying to connect", res);
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break; // success
		perror("failed to connect");
		close(sockfd);	// ignore this one
	}
	while ((res = res->ai_next) != NULL);

	if (res == NULL)
	{
		std::cerr << "failed to connect " << hostname << ", " << port << std::endl,
		sockfd = -1;
	}
	else
		print_address("using address", res);

	freeaddrinfo(ressave);

	return sockfd;
}
