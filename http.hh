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

class http_message
{
public:
	/* supported protocol */
	static const std::string protocol;

	/* constructor */
	http_message(http_method method, std::string filename, std::string host, std::string username);

	void set_header(std::string header);

	/* set payload for message */
	void set_payload(std::string payload);

	/* returns the whole message */
	std::string to_string() const;

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
