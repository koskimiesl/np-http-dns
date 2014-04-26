/* Minimal DNS query implementation */

#ifndef NETPROG_DNS_HH
#define NETPROG_DNS_HH

#include <cstdint>
#include <string>

#define SQUERYTYPE "A" // supported DNS query type

struct dns_header
{
	uint16_t id; // message identifier (16 bits)

	uint8_t qr; // query or response (1 bit, idx 8)
	uint8_t opcode; // type of query (4 bits, idx 4-7)
	uint8_t aa; // authoritative answer (1 bit, idx 3)
	uint8_t tc; // message truncated (1 bit, idx 2)
	uint8_t rd; // recursion desired (1 bit, idx 1)

	uint8_t ra; // recursion available (1 bit, idx 8)
	uint8_t z; // zero bit, ignored (1 bit, idx 7)
	uint8_t ad; // authenticated data (1 bit, idx 6)
	uint8_t cd; // checking disabled (1 bit, idx 5)
	uint8_t rcode; // response code (4 bits, idx 1-4)

	uint16_t qdcount; // number of question entries (16 bits)
	uint16_t ancount; // number of answer entries (16 bits)
	uint16_t nscount; // number of authority entries (16 bits)
	uint16_t arcount; // number of additional entries (16 bits)
};

struct dns_question
{
	char* qname;
	uint16_t qtype;
	uint16_t qclass;
};

struct dns_rfields
{
	uint16_t rtype;
	uint16_t rclass;
	uint16_t rttl;
	uint16_t rdlength;
};

struct dns_res_record
{
	unsigned char* rname;
	struct dns_rfields* rfields;
	unsigned char* rdata;
};

void do_dns_query(std::string queryname, std::string querytype);

#endif
