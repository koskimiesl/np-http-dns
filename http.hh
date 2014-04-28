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
	 * conf: HTTP configuration to use
	 * method: HTTP method
	 * dirpath: directory for files
	 * filename: filename (URI)
	 * hostname: host header field
	 * username: iam header field
	 * queryname: queryname for DNS request
	 * return: HTTP request object
	 */
	static http_request form_header(const http_conf& conf, std::string method, std::string dirpath, std::string filename,
									std::string hostname, std::string username, std::string queryname);

	/*
	 * Read HTTP request header from socket
	 *
	 * conf: HTTP configuration to use
	 * sockfd: socket descriptor with receive timeout set
	 * return: HTTP request object
	 */
	static http_request receive_header(const http_conf& conf, int sockfd);

	/*
	 * Print whole header and individual values
	 */
	void print_header() const;

	/*
	 * Write HTTP request to socket
	 *
	 * sockfd: socket descriptor
	 * dirpath: directory for files
	 * return: true on success, false on failure
	 */
	bool send(int sockfd, std::string dirpath) const;

	std::string header;
	http_method method;
	std::string uri;
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
	 * Class instances are created by static member functions
	 *
	 * conf: HTTP configuration to use
	 */
	http_request(const http_conf& conf);

	/*
	 * Form header string based on header field values
	 */
	void create_header();

	/*
	 * Form DNS query body for HTTP payload
	 *
	 * return: query body
	 */
	std::string get_query_body() const;

	/*
	 * Parse header field values from header string
	 *
	 * return: true on success, false on failure
	 */
	bool parse_header();

	const http_conf& conf; // reference to HTTP configuration
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
	 * conf: HTTP configuration to use
	 * sockf: socket descriptor
	 * req: HTTP request to process
	 * sevpath: path to serving directory
	 * username: iam header field
	 * return: HTTP response object
	 */
	static http_response proc_req_form_header(const http_conf& conf, int sockfd, http_request req, std::string servpath, std::string username);

	/*
	 * Read HTTP response from socket
	 *
	 * conf: HTTP configuration to use
	 * sockfd: socket descriptor
	 * reqmethod: original request method
	 * dirpath: directory for files
	 * filename: filename for payload
	 * return: HTTP response object
	 */
	static http_response receive(const http_conf& conf, int sockfd, http_method reqmethod, std::string dirpath, std::string filename);

	/*
	 * Create general purpose error message (404 Not Found)
	 *
	 * conf: HTTP configuration to use
	 * username: iam header field
	 * return: HTTP response object
	 */
	static http_response form_404_header(const http_conf& conf, std::string username);

	/*
	 * Print whole header and individual values
	 */
	void print_header() const;

	/*
	 * Print payload (if not a file)
	 */
	void print_payload() const;

	/*
	 * Write HTTP response to socket
	 *
	 * sockfd: socket descriptor
	 * servpath: path to serving directory
	 * return: true on success, false on failure
	 */
	bool send(int sockfd, std::string servpath) const;

	std::string header;
	http_protocol protocol;
	http_status status;
	std::string username;
	std::string content_type;
	size_t content_length;
	http_method request_method;
	std::string request_uri;
	std::string request_qname;
	std::string request_qtype;
	std::string dns_query_resp;

private:

	/*
	 * Private constructor
	 * Class instances are created by static member functions
	 *
	 * conf: HTTP configuration to use
	 */
	http_response(const http_conf& conf);

	/*
	 * Form header string based on header field values
	 */
	void create_header();

	/*
	 * Parse header field values from header string
	 *
	 * return: true on success, false on failure
	 */
	bool parse_header();

	/*
	 * Parse DNS query parameters from query body
	 *
	 * return: true on success, false on failure
	 */
	bool parse_req_query_params(const std::string& querybody);

	const http_conf& conf; // reference to HTTP configuration
};

#endif
