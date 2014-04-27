/* HTTP configuration related */

#ifndef NETPROG_HTTPCONF_HH
#define NETPROG_HTTPCONF_HH

#include <map>
#include <string>

/* supported protocol (version) */
typedef enum
{
	NOT_SET_PROT,
	HTTP_1_1, // HTTP/1.1
	UNSUPP_PROT
} http_protocol;

/* supported methods */
typedef enum
{
	NOT_SET_MET, // default value
	GET,
	PUT,
	POST,
	UNSUPP_MET
} http_method;

/* supported status codes */
typedef enum
{
	NOT_SET_ST, // default value
	OK_200,
	CREATED_201,
	BAD_REQUEST_400,
	FORBIDDEN_403,
	NOT_FOUND_404,
	UNSUPPORTED_MEDIA_TYPE_415,
	INTERNAL_ERROR_500,
	NOT_IMPLEMENTED_501,
	UNSUPP_ST
} http_status;

/* supported header fields */
typedef enum
{
	HOST,
	IAM,
	CONTENT_TYPE,
	CONTENT_LEN,
	UNSUPP_HF
} http_hfield;

/* class instead of global variables for thread-safety */
class http_conf
{
public:

	/*
	 * Constructor
	 */
	http_conf(const std::string dnsservip);

	http_protocol to_prot(std::string str) const;
	std::string to_str(http_protocol prot) const;

	http_method to_method(std::string str) const;
	std::string to_str(http_method method) const;

	http_status to_status(std::string str) const;
	std::string to_str(http_status status) const;

	http_hfield to_hfield(std::string str) const;
	std::string to_str(http_hfield hfield) const;

	const http_protocol protocol; // protocol (version) to use
	const std::string ctypegetput; // supported content type for GET and PUT
	const std::string ctypepost; // supported content type for POST
	const std::string uripost; // supported URI for POST
	const std::string delimiter; // delimiter between header and payload
	const std::string dnsservip; // DNS server to use (IPv4 address)

private:

	void init_maps();

	std::map<http_protocol, std::string> prot_to_str;
	std::map<std::string, http_protocol> str_to_prot;

	std::map<http_method, std::string> method_to_str;
	std::map<std::string, http_method> str_to_method;

	std::map<http_status, std::string> status_to_str;
	std::map<std::string, http_status> str_to_status;

	std::map<http_hfield, std::string> hfield_to_str;
	std::map<std::string, http_hfield> str_to_hfield;
};

#endif
