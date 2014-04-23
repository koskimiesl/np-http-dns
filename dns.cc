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
 * convert www.google.com to 3www6google3com
 */
void to_dns_name_enc(unsigned char* dns,unsigned char* host)
{
    int lock = 0 , i;
    strcat((char*)host,".");

    for(i = 0 ; i < strlen((char*)host) ; i++)
    {
        if(host[i]=='.')
        {
            *dns++ = i-lock;
            for(;lock<i;lock++)
            {
                *dns++=host[lock];
            }
            lock++;
        }
    }
    *dns++='\0';
}

unsigned char* serialize_header(unsigned char* destination, struct dns_header* source)
{
	uint16_t idnbo = htons(source->id); // id in network byte order
	printf("%x\n", idnbo);
	unsigned char lo = idnbo & 0xFF;
	unsigned char hi = idnbo >> 8;
	destination[0] = lo;
	destination[1] = hi;

	unsigned char qrtord = 0x00; // byte from qr to rd
	qrtord += source->qr;
	qrtord <<= 4;
	qrtord += source->opcode;
	qrtord <<= 1;
	qrtord += source->aa;
	qrtord <<= 1;
	qrtord += source->tc;
	qrtord <<= 1;
	qrtord += source->rd;
	destination[2] = qrtord;

	unsigned char ratorcode = 0x00; // byte from ra to rcode
	ratorcode += source->ra;
	ratorcode <<= 1;
	ratorcode += source->z;
	ratorcode <<= 1;
	ratorcode += source->ad;
	ratorcode <<= 1;
	ratorcode += source->cd;
	ratorcode <<= 4;
	ratorcode += source->rcode;
	destination[3] = ratorcode;

	uint16_t qdcountnbo = htons(source->qdcount);
	unsigned char lo2 = qdcountnbo & 0xFF;
	unsigned char hi2 = qdcountnbo >> 8;
	destination[4] = lo2;
	destination[5] = hi2;

	destination[6] = source->ancount;
	destination[8] = source->nscount;
	destination[10] = source->arcount;

	return &destination[12];
}

void send_query()
{
	unsigned char sendbuf[65536];

	unsigned char headerspace[sizeof(struct dns_header)]; // space for init header values
	struct dns_header* header = (struct dns_header*)&headerspace;

	//struct dns_header* header = (struct dns_header*)&sendbuf;

	header->id = 14;

	header->qr = 0; // this is a query
	header->opcode = 0; // this is a standard query
	header->aa = 0; // not authoritative
	header->tc = 0; // this message is not truncated
	header->rd = 1; // recursion desired

	header->ra = 0; // recursion not available
	header->z = 0;
	header->ad = 0;
	header->cd = 0;
	header->rcode = 0;

	header->qdcount = 1; // we have 1 question
	header->ancount = 0;
	header->nscount = 0;
	header->arcount = 0;


	// serialize header and point to the query portion
	unsigned char* qname = serialize_header(sendbuf, header);
	//unsigned char* qname =(unsigned char*)&sendbuf[sizeof(struct dns_header)];
	unsigned char host[] = "www.google.com";
	to_dns_name_enc(qname, host);

	struct dns_qfields* qinfo =(struct dns_qfields*)&sendbuf[12 + (strlen((const char*)qname) + 1)]; // fill it

	qinfo->qtype = htons(1); // type A
	qinfo->qclass = htons(1); //its internet



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
	if ((sent = sendto(sockfd, (char*)sendbuf, 12 + (strlen((const char*)qname)+1) + sizeof(struct dns_qfields), 0, (struct sockaddr*)&dest,sizeof(dest))) < 0)
	{
		perror("sendto failed");
	}
	std::cout << sent << " bytes sent to udp socket" << std::endl;
	close(sockfd);
}
