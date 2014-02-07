/* HTTP protocol */

#ifndef HTTP_HH
#define HTTP_HH

#include <string>

/* supported methods */
enum http_method
{
	GET,
	PUT
};

enum http_status_code
{
	_200_OK_,
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
	http_message(http_method method, std::string filename, std::string host, std::string username);

	/* parse response header and return its status code */
	bool parse_resp_header(std::string header);

	/* set payload for message */
	void set_payload(std::string payload);

	/* create and return message */
	std::string create_request() const;

	/* dump object member values */
	void dump_values() const;

	http_status_code status_code;

private:

	const std::string method_to_str(http_method m) const;
	static const char* method_strings[];

	http_method method;
	std::string filename;
	std::string host;
	std::string content_type;
	size_t content_length;
	std::string username;
	std::string payload;
};

#endif
