/* HTTP configuration related definitions and classes */

#ifndef NETPROG_HTTPCONF_HH
#define NETPROG_HTTPCONF_HH

#include <map>

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

/* class instead of global variables for thread-safety */
class http_conf
{
public:

	/* Constructor */
	http_conf();

	std::map<std::string, http_method> str_to_method;
	std::map<http_method, std::string> method_to_str;
	std::map<std::string, http_status> str_to_status;
	std::map<http_status, std::string> status_to_str;

private:

	void init_maps();
};

#endif
