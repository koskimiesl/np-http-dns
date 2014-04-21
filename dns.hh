
#ifndef NETPROG_DNS_HH
#define NETPROG_DNS_HH

struct dns_header
{
	uint16_t id; // message identifier (16 bits)

	uint8_t rd : 1; // recursion desired (1 bit)
	uint8_t tc : 1; // message truncated (1 bit)
	uint8_t aa : 1; // authoritative answer (1 bit)
	uint8_t opcode : 4; // type of query (4 bits)
	uint8_t qr : 1; // query or response (1 bit)

	uint8_t rcode : 4; // response code (4 bits)
	uint8_t cd : 1; // checking disabled (1 bit)
	uint8_t ad : 1; // authenticated data (1 bit)
	uint8_t z : 1; // zero bit, ignored (1 bit)
	uint8_t ra : 1; // recursion available (1 bit)

	uint16_t qdcount; // number of question entries (16 bits)
	uint16_t ancount; // number of answer entries (16 bits)
	uint16_t nscount; // number of authority entries (16 bits)
	uint16_t arcount; // number of additional entries (16 bits)
};

#endif
