/* Minimal DNS query implementation */

#ifndef NETPROG_DNS_HH
#define NETPROG_DNS_HH

#include <string>

#define SQUERYTYPE "A" // supported DNS query type

typedef enum
{
	SUCCESS,
	FAIL
} dns_query_status;

struct dns_query_response
{
	dns_query_status status;
	std::string response; // contains necessary data from answer resource records
	size_t resp_len; // response length in bytes
};

dns_query_response do_dns_query(std::string queryname, std::string querytype);

#endif
