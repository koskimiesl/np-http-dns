/* HTTP protocol */

#ifndef NETPROG_HTTP_HH
#define NETPROG_HTTP_HH

#include <string>

/* supported methods */
enum http_method
{
	NOT_SET, // default value
	GET,
	PUT,
	UNSUPPORTED
};

/* supported status codes */
enum http_status_code
{
	_NOT_SET_, // default value
	_200_OK_,
	_201_CREATED_,
	_404_NOT_FOUND_,
	_UNSUPPORTED_
};

class http_message
{
public:
	/* supported protocol */
	static const std::string protocol;

	/* constructors */
	http_message();
	http_message(std::string header);
	http_message(http_method method, std::string filename, std::string host, std::string username);
	http_message(http_method method, std::string filename, size_t filesize, const char* file, std::string host, std::string username);

	/* parse request header and return success */
	bool parse_req_header();

	/* parse response header and return success*/
	bool parse_resp_header(std::string header);

	/* create and return header */
	std::string create_header() const;

	std::string create_resp_header() const;

	/* dump object member values */
	void dump_values() const;

	std::string header; // whole header
	http_method method;
	const char* payload;
	http_status_code status_code;
	std::string filename;
	size_t content_length;

private:

	const std::string method_to_str(http_method m) const;
	static const char* method_strings[];
	static const char* status_code_strings[];

	std::string host;
	std::string content_type;
	std::string username;
	std::string server;
};

#endif
