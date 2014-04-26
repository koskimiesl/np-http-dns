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
#include "networking.hh"

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

/* Serializes data from header structure to buffer to be sent over network
 *
 * return: address in buffer to write next
 */
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
 * return: address in buffer to write next
 */
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

/* index of first bit (LSB) is 1 */
uint8_t get_bit(uint8_t byte, int bitidx)
{
	if (bitidx > 0 && bitidx <= 8)
    {
    	uint8_t bitmask = 1 << (bitidx - 1);
    	return (byte & bitmask) ? 1 : 0;
    }
    else
        return 0;
}

void recv_response(int sockfd, struct sockaddr_in* dest)
{
	/* read fixed size header (12 bytes) from socket */
	uint8_t headerbuf[12];
	ssize_t remaining = 12;
	size_t byteidx = 0;
	ssize_t read;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	while (remaining > 0 &&
		  (read = recvfrom(sockfd, &headerbuf[byteidx], remaining, 0, (struct sockaddr*)dest, &addrlen)) > 0)
	{
		byteidx += read;
		remaining -= read;
		std::cout << read << " bytes read, " << remaining << " bytes remaining from header" << std::endl;
	}
	if (read < 0)
	{
		perror("recvfrom");
		return;
	}
	if (remaining != 0)
	{
		std::cerr << "remaining is not zero" << std::endl;
		return;
	}
	uint8_t* hbufptr = headerbuf; // buffer address to read next

	/* construct header structure based on read data */
	struct dns_header header;

	/* convert id to host byte order, set to structure */
	uint16_t idnbo;
	memcpy(&idnbo, hbufptr, sizeof(uint16_t));
	header.id = ntohs(idnbo);
	std::cout << "read id: " << header.id << std::endl;
	hbufptr += sizeof(uint16_t);


	/* check byte from qr field to rd field, set bits to structure */

	uint8_t qrtord = *hbufptr;

	header.qr = get_bit(qrtord, 8);
	std::cout << "read qr: " << (unsigned short)header.qr << std::endl;

	uint8_t opcode = 0x00;
	opcode += get_bit(qrtord, 7);
	opcode <<= 1;
	opcode += get_bit(qrtord, 6);
	opcode <<= 1;
	opcode += get_bit(qrtord, 5);
	opcode <<= 1;
	opcode += get_bit(qrtord, 4);
	header.opcode = opcode;
	std::cout << "read opcode: " << (unsigned short)header.opcode << std::endl;

	header.aa = get_bit(qrtord, 3);
	std::cout << "read aa: " << (unsigned short)header.aa << std::endl;

	header.tc = get_bit(qrtord, 2);
	std::cout << "read tc: " << (unsigned short)header.tc << std::endl;

	header.rd = get_bit(qrtord, 1);
	std::cout << "read rd: " << (unsigned short)header.rd << std::endl;

	hbufptr += sizeof(uint8_t);


	/* check byte from ra field to rcode field, set bits to structure */

	uint8_t ratorcode = *hbufptr;

	header.ra = get_bit(ratorcode, 8);
	std::cout << "read ra: " << (unsigned short)header.ra << std::endl;

	header.z = get_bit(ratorcode, 7);
	std::cout << "read z: " << (unsigned short)header.z << std::endl;

	header.ad = get_bit(ratorcode, 6);
	std::cout << "read ad: " << (unsigned short)header.ad << std::endl;

	header.cd = get_bit(ratorcode, 5);
	std::cout << "read cd: " << (unsigned short)header.cd << std::endl;

	uint8_t rcode = 0x00;
	rcode += get_bit(ratorcode, 4);
	rcode <<= 1;
	rcode += get_bit(ratorcode, 3);
	rcode <<= 1;
	rcode += get_bit(ratorcode, 2);
	rcode <<= 1;
	rcode += get_bit(ratorcode, 1);
	header.rcode = rcode;
	std::cout << "read rcode: " << (unsigned short)header.rcode << std::endl;

	hbufptr += sizeof(uint8_t);


	/* convert qdcount to host byte order, set to structure */
	uint16_t qdcountnbo;
	memcpy(&qdcountnbo, hbufptr, sizeof(uint16_t));
	header.qdcount = ntohs(qdcountnbo);
	std::cout << "read qdcount: " << header.qdcount << std::endl;
	hbufptr += sizeof(uint16_t);

	/* convert ancount to host byte order, set to structure */
	uint16_t ancountnbo;
	memcpy(&ancountnbo, hbufptr, sizeof(uint16_t));
	header.ancount = ntohs(ancountnbo);
	std::cout << "read ancount: " << header.ancount << std::endl;
	hbufptr += sizeof(uint16_t);

	/* convert nscount to host byte order, set to structure */
	uint16_t nscountnbo;
	memcpy(&nscountnbo, hbufptr, sizeof(uint16_t));
	header.nscount = ntohs(nscountnbo);
	std::cout << "read nscount: " << header.nscount << std::endl;
	hbufptr += sizeof(uint16_t);

	/* convert arcount to host byte order, set to structure */
	uint16_t arcountnbo;
	memcpy(&arcountnbo, hbufptr, sizeof(uint16_t));
	header.arcount = ntohs(arcountnbo);
	std::cout << "read arcount: " << header.arcount << std::endl;
	hbufptr += sizeof(uint16_t);

	close(sockfd);
}

void do_dns_query(std::string queryname, std::string querytype)
{
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

	int sockfd;
	struct sockaddr_in dest;
	if ((sockfd = init_udp(&dest, "8.8.8.8", 53)) < 0)
		return;

	ssize_t sent;
	if ((sent = sendto(sockfd, sendbuf, msglen, 0, (struct sockaddr*)&dest, sizeof(dest))) < 0)
	{
		perror("sendto");
		close(sockfd);
		return;
	}
	std::cout << sent << " bytes sent to udp socket" << std::endl;

	recv_response(sockfd, &dest);
}
