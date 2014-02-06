#include <sstream>

#include "http.hh"

const std::string http_message::protocol = "HTTP/1.1";
const char* http_message::method_strings[] = {"GET", "PUT"};

http_message::http_message(http_method m, std::string f, std::string h, std::string u)
 : method(m), filename(f), host(h), username(u)
{ }

void http_message::set_header(std::string header)
{
	return;
}

std::string http_message::to_string() const
{
	std::stringstream ss;
	ss << method_to_str(method) << " " << filename << " " << protocol << "\r\n";
	ss << "Host: " << host << "\r\n";
	ss << "Iam: " << username << "\r\n\r\n";
	std::string str = ss.str();
	return str;
}

const std::string http_message::method_to_str(http_method m) const
{
	return method_strings[m];
}
