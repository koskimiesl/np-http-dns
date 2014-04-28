/* Minimal DNS query implementation */

#ifndef NETPROG_DNS_HH
#define NETPROG_DNS_HH

#include <string>

#define SQUERYTYPE "A" // supported DNS query type

/* DNS query status */
typedef enum
{
	SUCCESS,
	FAIL
} dns_query_status;

/* DNS query response */
struct dns_query_response
{
	dns_query_status status;
	std::string response; // contains necessary data from answer resource records
	size_t resp_len; // response length in bytes
};

/*
 * Perform a DNS query
 *
 * dnsservip: IP of DNS server to use
 * queryname: name to be queried
 * querytype: query type
 * return: DNS query response structure
 */
dns_query_response do_dns_query(std::string dnsservip, std::string queryname, std::string querytype);

#endif
