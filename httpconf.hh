/* HTTP configuration related */

#ifndef NETPROG_HTTPCONF_HH
#define NETPROG_HTTPCONF_HH

#include <map>

/* supported protocol */
typedef enum
{
	NOT_SET_PROT,
	HTTP_1_1, // HTTP/1.1
	UNSUPP_PROT
} http_protocol;

/* supported methods */
typedef enum
{
	NOT_SET, // default value
	GET,
	PUT,
	POST,
	UNSUPPORTED
} http_method;

/* supported status codes */
typedef enum
{
	_NOT_SET_, // default value
	_200_OK_,
	_201_CREATED_,
	_400_BAD_REQUEST_,
	_403_FORBIDDEN_,
	_404_NOT_FOUND_,
	_415_UNSUPPORTED_MEDIA_TYPE_,
	_500_INTERNAL_ERROR_,
	_501_NOT_IMPLEMENTED_,
	_UNSUPPORTED_
} http_status;

/* supported header fields */
typedef enum
{
	HOST,
	IAM,
	CONTENT_TYPE,
	CONTENT_LEN
} http_hfield;

/* class instead of global variables for thread-safety */
class http_conf
{
public:

	/*
	 * Constructor
	 */
	http_conf();


	std::map<http_method, std::string> method_to_str;
	std::map<std::string, http_status> str_to_status;
	std::map<http_status, std::string> status_to_str;

	http_protocol to_prot(std::string str) const;

	http_method to_method(std::string str) const;

	http_hfield to_hfield(std::string str) const;

private:

	void init_maps();

	std::map<std::string, http_method> str_to_method;

	std::map<http_protocol, std::string> prot_to_str;
	std::map<std::string, http_protocol> str_to_prot;

	std::map<std::string, http_hfield> str_to_hfield;

};

#endif
