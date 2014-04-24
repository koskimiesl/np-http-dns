#include <arpa/inet.h>
#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>
#include <sys/socket.h>
#include <unistd.h>

#include "dns.hh"

/*
 * convert hostname to DNS encoding
 */
void to_dns_name_enc(char* dnsformat, char* hostformat)
{
    size_t lock = 0, i;
    strcat(hostformat, ".");

    for (i = 0; i < strlen(hostformat); i++)
    {
        if (hostformat[i] == '.')
        {
            *dnsformat++ = i - lock;
            for (; lock < i; lock++)
            {
                *dnsformat ++= hostformat[lock];
            }
            lock++;
        }
    }
    *dnsformat ++= '\0';
}

void init_query_header(dns_header* header)
{
	srand(time(NULL));
	int id = rand() % 65536; // random integer between 0 and 65535
	header->id = (uint16_t)id;

	header->qr = 0; // query
	header->opcode = 0; // standard query
	header->aa = 0; // not authoritative
	header->tc = 0; // not truncated
	header->rd = 1; // recursion desired

	header->ra = 0;
	header->z = 0;
	header->ad = 0;
	header->cd = 0;
	header->rcode = 0;

	header->qdcount = 1; // 1 question
	header->ancount = 0;
	header->nscount = 0;
	header->arcount = 0;
}

void init_query_question(dns_question* question, std::string queryname)
{
	char host[300];
	strcpy(host, queryname.c_str());
	char qname[300];
	to_dns_name_enc(qname, host);

	question->qname = qname;
	question->qtype = 1; // type A
	question->qclass = 1; // class IN
}

/* Copies data from header structure to buffer to be sent over network
 *
 * return: address in buffer to write next */
uint8_t* serialize_header(uint8_t* buffer, dns_header* source, size_t& msglen)
{
	uint8_t* bufptr = buffer; // address to write next
	size_t n; // number of bytes to write

	/* convert id field to network byte order, copy to buffer */
	uint16_t idnbo = htons(source->id);
	n = sizeof(uint16_t);
	memcpy(bufptr, &idnbo, n);
	bufptr += n;
	msglen += n;

	/* construct the byte from qr field to rd field, copy to buffer */
	uint8_t qrtord = 0x00;
	qrtord += source->qr;
	qrtord <<= 4;
	qrtord += source->opcode;
	qrtord <<= 1;
	qrtord += source->aa;
	qrtord <<= 1;
	qrtord += source->tc;
	qrtord <<= 1;
	qrtord += source->rd;
	n = sizeof(uint8_t);
	memcpy(bufptr, &qrtord, n);
	bufptr += n;
	msglen += n;

	/* construct the byte from ra field to rcode field, copy to buffer */
	uint8_t ratorcode = 0x00;
	ratorcode += source->ra;
	ratorcode <<= 1;
	ratorcode += source->z;
	ratorcode <<= 1;
	ratorcode += source->ad;
	ratorcode <<= 1;
	ratorcode += source->cd;
	ratorcode <<= 4;
	ratorcode += source->rcode;
	n = sizeof(uint8_t);
	memcpy(bufptr, &ratorcode, n);
	bufptr += n;
	msglen += n;


	/* convert count fields to network byte order, copy to buffer */

	n = sizeof(uint16_t);

	uint16_t qdcountnbo = htons(source->qdcount);
	memcpy(bufptr, &qdcountnbo, n);
	bufptr += n;
	msglen += n;

	uint16_t ancountnbo = htons(source->ancount);
	memcpy(bufptr, &ancountnbo, n);
	bufptr += n;
	msglen += n;

	uint16_t nscountnbo = htons(source->nscount);
	memcpy(bufptr, &nscountnbo, n);
	bufptr += n;
	msglen += n;

	uint16_t arcountnbo = htons(source->arcount);
	memcpy(bufptr, &arcountnbo, n);
	bufptr += n;
	msglen += n;

	return bufptr;
}

/* Copies data from question structure into buffer to be sent over network
 *
 * return: address in buffer to write next */
uint8_t* serialize_question(uint8_t* buffer, dns_question* source, size_t& msglen)
{
	uint8_t* bufptr = buffer; // address to write next
	size_t n; // number of bytes to write

	/* copy qname string to buffer */
	n = strlen(source->qname) + 1; // include terminating null character
	memcpy(bufptr, source->qname, n);
	bufptr += n;
	msglen += n;

	n = sizeof(uint16_t);

	/* convert qtype field to network byte order, copy to buffer */
	uint16_t qtypenbo = htons(source->qtype);
	memcpy(bufptr, &qtypenbo, n);
	bufptr += n;
	msglen += n;

	/* convert qclass field to network byte order, copy to buffer */
	uint16_t qclassnbo = htons(source->qclass);
	memcpy(bufptr, &qclassnbo, n);
	bufptr += n;
	msglen += n;

	return bufptr;
}

void send_query(std::string queryname, std::string querytype)
{
	std::cout << "querytype: " << querytype << std::endl;
	if (!querytype.compare(SQUERYTYPE)) // only type A currently supported
	{
		std::cerr << "unsupported DNS query type" << std::endl;
		return;
	}

	if (queryname.length() > 200) // arbitrary maximum length
	{
		std::cerr << "too long query name" << std::endl;
		return;
	}

	uint8_t sendbuf[65536];
	uint8_t* bufptr = sendbuf; // address to write next
	size_t msglen = 0;

	/* init header structure, serialize for sending to network */
	dns_header header;
	init_query_header(&header);
	bufptr = serialize_header(bufptr, &header, msglen);

	/* init question structure, serialize for sending to network */
	dns_question question;
	init_query_question(&question, queryname);
	bufptr = serialize_question(bufptr, &question, msglen);

	struct sockaddr_in dest;
	memset(&dest, 0, sizeof(dest));
	dest.sin_family = AF_INET;
	dest.sin_port = htons(53);
	dest.sin_addr.s_addr = inet_addr("8.8.8.8");

	ssize_t sent;
	int sockfd;
	if ((sockfd = socket(AF_INET , SOCK_DGRAM , 0)) < 0)
	{
		perror("socket");
		return;
	}

	std::cout << "using sockfd " << sockfd << std::endl;
	if ((sent = sendto(sockfd, sendbuf, msglen, 0, (struct sockaddr*)&dest, sizeof(dest))) < 0)
	{
		perror("sendto failed");
	}
	std::cout << sent << " bytes sent to udp socket" << std::endl;
	close(sockfd);
}
