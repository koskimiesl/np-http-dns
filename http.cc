#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include "http.hh"

const std::string http_message::protocol = "HTTP/1.1";
const char* http_message::method_strings[] = {"GET", "PUT"};

http_message::http_message()
{ }

http_message::http_message(http_method m, std::string f, std::string h, std::string u)
 : method(m), filename(f), host(h), username(u)
{ }

bool http_message::parse_resp_header(std::string header)
{
	using namespace std;
	istringstream headeriss(header);
	string line;
	bool contentlen = false; // is content length parsed successfully?
	status_code = http_status_code::_UNSUPPORTED_;
	while (getline(headeriss, line))
	{
		istringstream lineiss(line);

		// tokens separated by a white space into a vector
		vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
		if (tokens.size() > 0)
		{
			vector<string>::const_iterator it = tokens.begin();
			if (*it == "HTTP/1.1") // read two more tokens
			{
				string code, msg; // status code
				if (it++ != tokens.end())
				{
					code = *it;
					if (it++ != tokens.end())
						msg = *it;
				}
				if (code == "200" && msg == "OK")
					status_code = http_status_code::_200_OK_;
				else if (code == "404" && msg == "Not Found")
					status_code = http_status_code::_404_NOT_FOUND_;
			}
			else if (*it == "Content-Length:") // read one more token
			{
				if (it++ != tokens.end())
				{
					istringstream iss(*it);
					iss >> content_length; // convert to size_t
					contentlen = true;
				}
			}
			else if (*it == "Content-Type:") // read one more token
			{
				if (it++ != tokens.end())
					content_type = *it;
			}
			else if (*it == "Iam:") // read one more token
			{
				if (it++ != tokens.end())
					username = *it;
			}
			else if (*it == "Server:") // read one more token
			{
				if (it++ != tokens.end())
					server = *it;
			}
		}
	}
	if (!contentlen)
		return false;
	return true;
}

std::string http_message::create_request() const
{
	std::stringstream ss;
	ss << method_to_str(method) << " " << filename << " " << protocol << "\r\n";
	ss << "Host: " << host << "\r\n";
	ss << "Iam: " << username << "\r\n\r\n";
	std::string str = ss.str();
	return str;
}

void http_message::dump_values() const
{
	std::cout << "method: " << method << std::endl
			  << "filename: " << filename << std::endl
			  << "status code: " << status_code << std::endl
			  << "host: " << host << std::endl
			  << "content type: " << content_type << std::endl
			  << "content length: " << content_length << std::endl
			  << "username: " << username << std::endl
			  << "server: " << server << std::endl
			  << "payload: " << payload << std::endl;
}

const std::string http_message::method_to_str(http_method m) const
{
	return method_strings[m];
}
