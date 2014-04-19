/* HTTP protocol */

#ifndef NETPROG_HTTP_HH
#define NETPROG_HTTP_HH

#include <stdexcept>
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
	_400_BAD_REQUEST_,
	_403_FORBIDDEN_,
	_404_NOT_FOUND_,
	_415_UNSUPPORTED_MEDIA_TYPE_,
	_500_INTERNAL_ERROR_,
	_501_NOT_IMPLEMENTED_,
	_UNSUPPORTED_
};

class http_request
{
public:

	/* Constructor */
	http_request();

	/* Create HTTP request header by constructing it from parameters
	 *
	 * param method:
	 * param filename:
	 * param hostname:
	 * param username:
	 * returns: http_request object */
	static http_request form_header(std::string method, std::string dirpath, std::string filename, std::string hostname, std::string username);

	/* Read HTTP request header from socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd: socket descriptor with receive timeout set
	 * returns: request object */
	static http_request receive_header(int sockfd);

	/* Print whole header and parsed values */
	void print_header() const;

	/* Write HTTP request to socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd:
	 * param dirpath:
	 * return: true on success, false on failure */
	bool send(int sockfd, std::string dirpath) const;

	std::string header; // whole header

	http_method method;
	std::string filename;
	std::string protocol;
	std::string hostname;
	std::string username;
	std::string content_type;
	size_t content_length;

private:

	void create_header();
	void parse_header();
};

class http_response
{
public:

	/* Constructor */
	http_response();

	/* Process HTTP request and create response header
	 *
	 * param request:
	 * param sevpath:
	 * param username:
	 * return: http_response object */
	static http_response proc_req_and_form_header(int sockfd, http_request request, std::string servpath, std::string username);

	/* Read HTTP response from socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd: socket descriptor
	 * return: http_response object */
	static http_response receive(int sockfd, http_method reqmethod, std::string dirpath, std::string filename);

	/* Print whole header and parsed values */
	void print_header() const;

	/* Write HTTP response to socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd:
	 * param servpath:
	 * return: true on success, false on failure */
	bool send(int sockfd, std::string servpath) const;

	std::string header; // whole header

	std::string protocol;
	http_status_code status_code;
	std::string username;
	std::string content_type;
	size_t content_length;

	http_method request_method;
	std::string request_filename;

private:

	void create_header();
	void parse_header();
};

class http_exception : public std::runtime_error
{
public:

  http_exception();
};

#endif
