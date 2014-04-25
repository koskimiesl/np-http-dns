/* HTTP protocol */

#ifndef NETPROG_HTTP_HH
#define NETPROG_HTTP_HH

#include <stdexcept>
#include <string>

#include "httpconf.hh"

/*
 * HTTP request
 */
class http_request
{
public:

	/*
	 * Create HTTP request header by constructing it from parameters
	 *
	 * conf:
	 * method:
	 * dirpath:
	 * filename:
	 * hostname:
	 * username:
	 * queryname:
	 * return: http_request object
	 */
	static http_request form_header(const http_conf& conf, std::string method, std::string dirpath, std::string filename,
									std::string hostname, std::string username, std::string queryname);

	/*
	 * Read HTTP request header from socket
	 * Caller must open and close the socket descriptor
	 *
	 * conf:
	 * sockfd: socket descriptor with receive timeout set
	 * return: request object
	 */
	static http_request receive_header(const http_conf& conf, int sockfd);

	/*
	 * Print whole header and parsed values
	 */
	void print_header() const;

	/*
	 * Write HTTP request to socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd:
	 * param dirpath:
	 * return: true on success, false on failure
	 */
	bool send(int sockfd, std::string dirpath) const;

	std::string header; // whole header
	http_method method;
	std::string filename;
	http_protocol protocol;
	std::string hostname;
	std::string username;
	std::string content_type;
	size_t content_length;
	std::string queryname;
	std::string querytype;

private:

	/*
	 * Private constructor
	 * Class instances created by static member functions
	 *
	 * conf: reference to HTTP configuration
	 */
	http_request(const http_conf& conf);

	void create_header();
	std::string get_query_body() const;
	bool parse_header();

	const http_conf& conf; // reference to configuration
};

/*
 * HTTP response
 */
class http_response
{
public:

	/*
	 * Process HTTP request and create response header
	 *
	 * param request:
	 * param sevpath:
	 * param username:
	 * return: http_response object
	 */
	static http_response proc_req_form_header(const http_conf& conf, int sockfd, http_request req, std::string servpath, std::string username);

	/*
	 * Read HTTP response from socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd: socket descriptor
	 * return: http_response object
	 */
	static http_response receive(const http_conf& conf, int sockfd, http_method reqmethod, std::string dirpath, std::string filename);

	/*
	 * Print whole header and parsed values
	 */
	void print_header() const;

	/* Write HTTP response to socket
	 * Caller must open and close the socket descriptor
	 *
	 * param sockfd:
	 * param servpath:
	 * return: true on success, false on failure
	 */
	bool send(int sockfd, std::string servpath) const;

	std::string header; // whole header
	http_protocol protocol;
	http_status status_code;
	std::string username;
	std::string content_type;
	size_t content_length;
	http_method request_method;
	std::string request_filename;
	std::string request_qname;
	std::string request_qtype;

private:

	/*
	 * Private constructor
	 * Class instances created by static member functions
	 *
	 * conf: reference to HTTP configuration
	 */
	http_response(const http_conf& conf);

	void create_header();
	void parse_header();
	bool parse_req_query_params(const std::string& querybody);

	const http_conf& conf; // reference to configuration
};

class http_exception : public std::runtime_error
{
public:

  http_exception(const std::string message);
};

#endif
