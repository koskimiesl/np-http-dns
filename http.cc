#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "http.hh"

const std::string http_message::protocol = "HTTP/1.1";
const char* http_message::method_strings[] = {"NOT SET", "GET", "PUT", "UNSUPPORTED"};
const char* http_message::status_code_strings[] = {"NOT SET", // default value
												   "200 OK",
												   "201 Created",
												   "400 Bad Request",
												   "403 Forbidden",
												   "404 Not Found",
												   "501 Not Implemented",
												   "UNSUPPORTED"};

http_message::http_message()
{
	method = http_method::NOT_SET;
	status_code = http_status_code::_NOT_SET_;
}

http_message::http_message(std::string header) : header(header)
{
	method = http_method::NOT_SET;
	status_code = http_status_code::_NOT_SET_;
}

http_message::http_message(http_method m, std::string f, std::string h, std::string u)
 : method(m), filename(f), host(h), username(u)
{
	status_code = http_status_code::_NOT_SET_;
}

http_message::http_message(http_method m, std::string f, size_t s, const char* p, std::string h, std::string u)
 : method(m), filename(f), host(h), username(u)
{
	payload = p;
	content_length = s;
	status_code = http_status_code::_NOT_SET_;
}

void http_message::parse_req_header()
{
	using namespace std;
	istringstream headeriss(header);
	string line; // single line in header
	if (!getline(headeriss, line))
	{
		status_code = http_status_code::_400_BAD_REQUEST_;
		return;
	}
	istringstream lineiss(line);
	// tokens separated by a white space into a vector
	vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
	if (tokens.size() == 0)
	{
		status_code = http_status_code::_400_BAD_REQUEST_;
		return;
	}
	vector<string>::const_iterator it = tokens.begin();

	if (*it == "GET") // parse GET header
	{
		method = http_method::GET;
		if (it++ != tokens.end())
		{
			if (access((*it).c_str(), R_OK) < 0) // check file existence and read permission
			{
				perror("access");
				status_code = http_status_code::_404_NOT_FOUND_;
				return;
			}
			filename = *it;
			std::ifstream fs(filename);
			if (!fs.good()) // check stream state
			{
				std::cerr << "file stream error" << std::endl;
				status_code = http_status_code::_404_NOT_FOUND_;
				return;
			}

			// determine file size (i.e. content length)
			fs.seekg(0, fs.end);
			int length = fs.tellg();
			fs.close();
			content_length = length;

			if (it++ != tokens.end() && *it == protocol)
			{
				status_code = http_status_code::_200_OK_;
				return;
			}
		}
	}

	else if (*it == "PUT") // parse PUT header
	{
		method = http_method::PUT;
		if (it++ != tokens.end())
		{
			filename = *it;
			if (access(filename.c_str(), F_OK) != -1) // file exists
			{
				if (access(filename.c_str(), W_OK) != -1) // program has write permission
					status_code = http_status_code::_200_OK_;
				else
					status_code = http_status_code::_403_FORBIDDEN_;
			}
			else // new file will be created
				status_code = http_status_code::_201_CREATED_;

			if (!(it++ != tokens.end() && *it == protocol))
			{
				status_code = http_status_code::_400_BAD_REQUEST_;
				return;
			}
		}
		bool typegiven = false;
		bool lengthgiven = false;
		while (getline(headeriss, line)) // parse other required values
		{
			istringstream lineiss(line);
			// tokens separated by a white space into a vector
			vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
			if (tokens.size() > 0)
			{
				vector<string>::const_iterator it = tokens.begin();
				if (*it == "Content-Type:")
				{
					if (!(it++ != tokens.end() && *it == "text/plain"))
					{
						status_code = http_status_code::_501_NOT_IMPLEMENTED_;
						return;
					}
					typegiven = true;
				}
				else if (*it == "Content-Length:")
				{
					if (it++ != tokens.end())
					{
						istringstream iss(*it);
						iss >> content_length; // convert to size_t
						lengthgiven = true;
					}
				}
			}
		}
		if (!typegiven || !lengthgiven)
			status_code = http_status_code::_400_BAD_REQUEST_;
	}

	else // method not implemented
		status_code = http_status_code::_501_NOT_IMPLEMENTED_;
}

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
			if (*it == "HTTP/1.1")
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
				else if (code == "201" && msg == "Created")
					status_code = http_status_code::_201_CREATED_;
				else if (code == "404" && msg == "Not")
				{
					if (it++ != tokens.end() && *it == "Found")
						status_code = http_status_code::_404_NOT_FOUND_;
				}
			}
			else if (*it == "Content-Length:")
			{
				if (it++ != tokens.end())
				{
					istringstream iss(*it);
					iss >> content_length; // convert to size_t
					contentlen = true;
				}
			}
			else if (*it == "Content-Type:")
			{
				if (it++ != tokens.end())
					content_type = *it;
			}
			else if (*it == "Iam:")
			{
				if (it++ != tokens.end())
					username = *it;
			}
			else if (*it == "Server:")
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

std::string http_message::create_header() const
{
	std::stringstream ss;
	ss << method_to_str(method) << " " << filename << " " << protocol << "\r\n";
	ss << "Host: " << host << "\r\n";
	if (method == http_method::PUT)
	{
		ss << "Content-Type: text/plain" << "\r\n";
		ss << "Content-Length: " << content_length << "\r\n";
	}
	ss << "Iam: " << username << "\r\n\r\n";
	std::string str = ss.str();
	return str;
}

std::string http_message::create_resp_header(std::string username) const
{
	std::stringstream ss;
	ss << protocol << " " << status_code_strings[status_code] << "\r\n";
	if (method == http_method::GET)
	{
		if (status_code == http_status_code::_200_OK_)
		{
			ss << "Content-Type: text/plain" << "\r\n";
			ss << "Content-Length: " << content_length << "\r\n";
		}
	}
	ss << "Iam: " << username << "\r\n\r\n";
	return ss.str();
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
			  << "server: " << server << std::endl;
}

const std::string http_message::method_to_str(http_method m) const
{
	return method_strings[m];
}
