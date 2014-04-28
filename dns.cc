#include <arpa/inet.h>
#include <inttypes.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include "dns.hh"
#include "networking.hh"

#define UDPBUFSIZE 2048 // maximum datagram size sent and received
#define DNSPORT "53" // well-known DNS port number

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

struct dns_res_record
{
	std::string rname;
	uint16_t rtype;
	uint16_t rclass;
	uint32_t rttl;
	uint16_t rdlength;
	std::vector<uint8_t> rdata; // only IPv4 address supported
};

/* function prototypes */
bool send_query(int sockfd, struct sockaddr* destaddr, socklen_t addrlen, std::string queryname);
bool recv_response(int sockfd, std::string& formedresp);
void init_query_header(dns_header* header);
void init_query_question(dns_question* question, std::string queryname);
uint8_t* serialize_header(uint8_t* buffer, dns_header* source, size_t& msglen);
uint8_t* serialize_question(uint8_t* buffer, dns_question* source, size_t& msglen);
uint8_t* deserialize_header(uint8_t* headerstart, dns_header* header);
uint8_t* deserialize_question(uint8_t* msgstart, uint8_t* quesstart, dns_question* question);
uint8_t* deserialize_res_rec(uint8_t* msgstart, uint8_t* rrstart, dns_res_record* resrec, bool& supported);
std::string form_response(const std::vector<dns_res_record>& answers);
std::string remove_last_dot(std::string str);
std::string addr_type_to_str(uint16_t addrtype);
std::string addr_class_to_str(uint16_t addrclass);
std::string ipv4_addr_to_str(std::vector<uint8_t> addrdata);
void to_dns_name_enc(char* dnsformat, char* hostformat);
uint8_t* process_name(uint8_t *bstart, uint8_t *bcur, char *name);
uint8_t get_bit(uint8_t byte, int bitidx);

dns_query_response do_dns_query(std::string dnsservip, std::string queryname, std::string querytype)
{
	dns_query_response resp;

	if (!querytype.compare(SQUERYTYPE)) // only type A currently supported
	{
		std::cerr << "unsupported DNS query type" << std::endl;
		resp.status = dns_query_status::FAIL;
		return resp;
	}
	if (queryname.length() > 200) // arbitrary maximum length
	{
		std::cerr << "too long query name" << std::endl;
		resp.status = dns_query_status::FAIL;
		return resp;
	}

	int sockfd;
	struct sockaddr* destaddr;
	socklen_t addrlen;

	if ((sockfd = init_udp(dnsservip.c_str(), DNSPORT, &destaddr, &addrlen)) < 0)
	{
		resp.status = dns_query_status::FAIL;
		return resp;
	}

	if (!send_query(sockfd, destaddr, addrlen, queryname))
	{
		if (close(sockfd) < 0) perror("close");
		resp.status = dns_query_status::FAIL;
		return resp;
	}

	std::string formedresp; // string to be returned as a response
	if (!recv_response(sockfd, formedresp))
	{
		if (close(sockfd) < 0) perror("close");
		resp.status = dns_query_status::FAIL;
		return resp;
	}

	if (close(sockfd) < 0) perror("close");

	resp.status = dns_query_status::SUCCESS;
	resp.response = formedresp;
	resp.resp_len = strlen(resp.response.c_str());
	return resp;
}

bool send_query(int sockfd, struct sockaddr* destaddr, socklen_t addrlen, std::string queryname)
{
	uint8_t msg[UDPBUFSIZE];
	uint8_t* msgcur = msg; // address to write next
	size_t msglen = 0;

	/* init header structure, serialize for sending to network */
	dns_header header;
	init_query_header(&header);
	msgcur = serialize_header(msgcur, &header, msglen);

	/* init question structure, serialize for sending to network */
	dns_question question;
	init_query_question(&question, queryname);
	msgcur = serialize_question(msgcur, &question, msglen);

	ssize_t sent;
	if ((sent = sendto(sockfd, msg, msglen, 0, destaddr, addrlen)) < 0)
	{
		perror("sendto");
		return false;
	}
	std::cout << "UDP message of " << sent << " bytes sent" << std::endl;
	return true;
}

bool recv_response(int sockfd, std::string& formedresp)
{
	uint8_t udpmsg[UDPBUFSIZE];
	ssize_t recvd;
	if ((recvd = recvfrom(sockfd, udpmsg, UDPBUFSIZE, 0, NULL, NULL)) < 0)
	{
		perror("recvfrom");
		return false;
	}
	if (recvd == 0)
	{
		std::cerr << "no data received" << std::endl;
		return false;
	}
	std::cout << "UDP message of " << recvd << " bytes received" << std::endl;

	uint8_t* msgcur = udpmsg; // byte to read next from message

	/* construct header structure based on received data */
	struct dns_header header;
	msgcur = deserialize_header(msgcur, &header);

	/* check response code */
	if (header.rcode != 0)
		return false; // error, no need to deserialize the rest

	/* construct question structures based on received data */
	std::vector<dns_question> questions;
	uint16_t qi;
	for (qi = 0; qi < header.qdcount; qi++)
	{
		dns_question question;
		msgcur = deserialize_question(udpmsg, msgcur, &question);
		questions.push_back(question);
	}

	/* construct resource record structures for supported answer types based on received data */
	std::vector<dns_res_record> answerresrecs;
	uint16_t ai;
	for (ai = 0; ai < header.ancount; ai++)
	{
		dns_res_record answerresrec;
		bool supported;
		msgcur = deserialize_res_rec(udpmsg, msgcur, &answerresrec, supported);
		if (supported)
			answerresrecs.push_back(answerresrec);
	}

	/* form response string from answers */
	formedresp = form_response(answerresrecs);

	return true;
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

uint8_t* deserialize_header(uint8_t* headerstart, dns_header* header)
{
	std::cout << std::endl << "deserializing DNS header:" << std::endl;
	uint8_t* msgcur = headerstart;

	/* convert id to host byte order, set to structure */
	uint16_t idnbo;
	memcpy(&idnbo, msgcur, sizeof(uint16_t));
	header->id = ntohs(idnbo);
	std::cout << "id: " << header->id << std::endl;
	msgcur += sizeof(uint16_t);

	/* check byte from qr field to rd field, set bits to structure */
	uint8_t qrtord = *msgcur;
	header->qr = get_bit(qrtord, 8);
	std::cout << "qr: " << (unsigned short)header->qr << std::endl;
	uint8_t opcode = 0x00;
	opcode += get_bit(qrtord, 7);
	opcode <<= 1;
	opcode += get_bit(qrtord, 6);
	opcode <<= 1;
	opcode += get_bit(qrtord, 5);
	opcode <<= 1;
	opcode += get_bit(qrtord, 4);
	header->opcode = opcode;
	std::cout << "opcode: " << (unsigned short)header->opcode << std::endl;
	header->aa = get_bit(qrtord, 3);
	std::cout << "aa: " << (unsigned short)header->aa << std::endl;
	header->tc = get_bit(qrtord, 2);
	std::cout << "tc: " << (unsigned short)header->tc << std::endl;
	header->rd = get_bit(qrtord, 1);
	std::cout << "rd: " << (unsigned short)header->rd << std::endl;
	msgcur += sizeof(uint8_t);


	/* check byte from ra field to rcode field, set bits to structure */
	uint8_t ratorcode = *msgcur;
	header->ra = get_bit(ratorcode, 8);
	std::cout << "ra: " << (unsigned short)header->ra << std::endl;
	header->z = get_bit(ratorcode, 7);
	std::cout << "z: " << (unsigned short)header->z << std::endl;
	header->ad = get_bit(ratorcode, 6);
	std::cout << "ad: " << (unsigned short)header->ad << std::endl;
	header->cd = get_bit(ratorcode, 5);
	std::cout << "cd: " << (unsigned short)header->cd << std::endl;
	uint8_t rcode = 0x00;
	rcode += get_bit(ratorcode, 4);
	rcode <<= 1;
	rcode += get_bit(ratorcode, 3);
	rcode <<= 1;
	rcode += get_bit(ratorcode, 2);
	rcode <<= 1;
	rcode += get_bit(ratorcode, 1);
	header->rcode = rcode;
	std::cout << "rcode: " << (unsigned short)header->rcode << std::endl;
	msgcur += sizeof(uint8_t);

	/* convert qdcount to host byte order, set to structure */
	uint16_t qdcountnbo;
	memcpy(&qdcountnbo, msgcur, sizeof(uint16_t));
	header->qdcount = ntohs(qdcountnbo);
	std::cout << "qdcount: " << header->qdcount << std::endl;
	msgcur += sizeof(uint16_t);

	/* convert ancount to host byte order, set to structure */
	uint16_t ancountnbo;
	memcpy(&ancountnbo, msgcur, sizeof(uint16_t));
	header->ancount = ntohs(ancountnbo);
	std::cout << "ancount: " << header->ancount << std::endl;
	msgcur += sizeof(uint16_t);

	/* convert nscount to host byte order, set to structure */
	uint16_t nscountnbo;
	memcpy(&nscountnbo, msgcur, sizeof(uint16_t));
	header->nscount = ntohs(nscountnbo);
	std::cout << "nscount: " << header->nscount << std::endl;
	msgcur += sizeof(uint16_t);

	/* convert arcount to host byte order, set to structure */
	uint16_t arcountnbo;
	memcpy(&arcountnbo, msgcur, sizeof(uint16_t));
	header->arcount = ntohs(arcountnbo);
	std::cout << "arcount: " << header->arcount << std::endl;
	msgcur += sizeof(uint16_t);

	return msgcur;
}

uint8_t* deserialize_question(uint8_t* msgstart, uint8_t* quesstart, dns_question* question)
{
	std::cout << std::endl << "deserializing DNS question:" << std::endl;
	uint8_t* msgcur = quesstart;

	/* process name */
	char qname[1024];
	msgcur = process_name(msgstart, msgcur, qname);
	question->qname = qname;
	std::cout << "qname: " << question->qname << std::endl;

	/* process type */
	uint16_t qtypenbo;
	memcpy(&qtypenbo, msgcur, sizeof(uint16_t));
	question->qtype = ntohs(qtypenbo);
	std::cout << "qtype: " << question->qtype << std::endl;
	msgcur += sizeof(uint16_t);

	/* process class */
	uint16_t qclassnbo;
	memcpy(&qclassnbo, msgcur, sizeof(uint16_t));
	question->qclass = ntohs(qclassnbo);
	std::cout << "qclass: " << question->qclass << std::endl;
	msgcur += sizeof(uint16_t);

	return msgcur;
}

uint8_t* deserialize_res_rec(uint8_t* msgstart, uint8_t* rrstart, dns_res_record* resrec, bool& supported)
{
	std::cout << std::endl << "deserializing DNS resource record:" << std::endl;
	uint8_t* msgcur = rrstart;

	/* process name */
	char rname[1024];
	msgcur = process_name(msgstart, msgcur, rname);
	resrec->rname = rname;
	std::cout << "rname: " << resrec->rname << std::endl;

	/* process type */
	uint16_t rtypenbo;
	memcpy(&rtypenbo, msgcur, sizeof(uint16_t));
	resrec->rtype = ntohs(rtypenbo);
	std::cout << "rtype: " << resrec->rtype << std::endl;
	msgcur += sizeof(uint16_t);

	/* process class */
	uint16_t rclassnbo;
	memcpy(&rclassnbo, msgcur, sizeof(uint16_t));
	resrec->rclass = ntohs(rclassnbo);
	std::cout << "rclass: " << resrec->rclass << std::endl;
	msgcur += sizeof(uint16_t);

	/* process ttl */
	uint32_t rttlnbo;
	memcpy(&rttlnbo, msgcur, sizeof(uint32_t));
	resrec->rttl = ntohl(rttlnbo);
	std::cout << "rttl: " << resrec->rttl << std::endl;
	msgcur += sizeof(uint32_t);

	/* process data length */
	uint16_t rdlengthnbo;
	memcpy(&rdlengthnbo, msgcur, sizeof(uint16_t));
	resrec->rdlength = ntohs(rdlengthnbo);
	std::cout << "rdlength: " << resrec->rdlength << std::endl;
	msgcur += sizeof(uint16_t);

	/* process data */
	if (resrec->rtype == 1 && resrec->rclass == 1 && resrec->rdlength == 4)
	{
		std::vector<uint8_t> rdata;
		int i;
		for (i = 0; i < 4; i++)
			rdata.push_back(msgcur[i]);
		resrec->rdata = rdata;
		std::cout << "rdata: " << ipv4_addr_to_str(resrec->rdata) << std::endl;
		msgcur += 4;
		supported = true;
	}
	else
	{
		std::cout << "unsupported combination of type, class and data length, skipping data" << std::endl;
		msgcur += resrec->rdlength; // skip unsupported data
		supported = false;
	}

	return msgcur;
}

std::string form_response(const std::vector<dns_res_record>& answers)
{
	std::stringstream ss;
	ss << "DNS answers:" << std::endl << std::endl;

	std::vector<dns_res_record>::const_iterator it;
	for (it = answers.begin(); it != answers.end(); it++)
	{
		ss << "Answer:" << std::endl
		   << "Name: " << remove_last_dot(it->rname) << std::endl
		   << "Type: " << addr_type_to_str(it->rtype) << std::endl
		   << "Class: " << addr_class_to_str(it->rclass) << std::endl
		   << "TTL: " << it->rttl << " seconds" << std::endl
		   << "Data: " << ipv4_addr_to_str(it->rdata) << std::endl << std::endl;
	}

	return ss.str();
}

std::string ipv4_addr_to_str(std::vector<uint8_t> addrdata)
{
	std::stringstream ss;
	ss << (unsigned short)addrdata[0] << "." << (unsigned short)addrdata[1] << "."
	   << (unsigned short)addrdata[2] << "." << (unsigned short)addrdata[3];
	return ss.str();
}

std::string remove_last_dot(std::string str)
{
	if (str.length() > 0)
	{
		std::string::iterator it = str.end() - 1;
		if (*it == '.') // remove last dot
			str.erase(it);
	}
	return str;
}

std::string addr_type_to_str(uint16_t addrtype)
{
	switch (addrtype)
	{
	case 1:
		return "A";
	default:
		return "UNSUPPORTED";
	}
}

std::string addr_class_to_str(uint16_t addrclass)
{
	switch (addrclass)
	{
	case 1:
		return "IN";
	default:
		return "UNSUPPORTED";
	}
}

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

/* Processes one string in a DNS resource record.
 *
 * bstart (in):	pointer to the start of the DNS message (UDP payload)
 * bcur (in):	pointer to the currently processed position in a message.
		This should point to the start of compressed or uncompressed
		name string.
 * name (out):	buffer for storing the name string in dot-separated format
 *
 * returns:	updated position of bcur, pointing to the next position
 *		following the name
 */
uint8_t* process_name(uint8_t *bstart, uint8_t *bcur, char *name)
{
	/* from course example */

	uint8_t *p = bcur;
	char strbuf[80];
	char *strp;
	int compressed = 0;
	name[0] = 0;

	do
	{
		strp = strbuf;

		if ((*p & 0xc0) == 0xc0) // first two bits are set => compressed format
		{
			uint16_t offset = (*p & 0x3f);
			offset = (offset << 8) + *(p+1);

			p = bstart + offset; // move the read pointer to the offset given in message

			/* adjustment of bcur must only be done once, in case there are multiple nested pointers in msg */
			if (!compressed)
				bcur += 2;
			compressed = 1;
		}
		else if (*p > 0)
		{
			/* strbuf contains one element of name, not full name */
			memcpy(strbuf, p+1, *p);
			strp += *p;
			p += *p + 1;

			/* adjustment of bcur based on string length is only done if it was not compressed,
			   otherwise it is assumed to be 16 bits always */
			if (!compressed)
				bcur = p;

			*strp = '.';
			*(strp+1) = 0;
			strcat(name, strbuf);
		}
	} while (*p > 0);

	if (!compressed)
		bcur++; // compensate for trailing 0 (unless name was compressed)

	return bcur;
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
