#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "http.hh"
#include "networking.hh"

#define RECVBUFSIZE 1024

static const std::string sprotocol = "HTTP/1.1"; // supported protocol
static const std::string scontenttype = "text/plain"; // supported content type
static const std::string delimiter = "\r\n\r\n";
static const char* method_strings[] = {"NOT SET", "GET", "PUT", "UNSUPPORTED"};

static std::map<std::string, http_method> stringtomethod = { {"NOT SET", http_method::NOT_SET},
															 {"GET", http_method::GET},
															 {"PUT", http_method::PUT},
															 {"UNSUPPORTED", http_method::UNSUPPORTED} };

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
			if ((*it).length() > 0)
			{
				if ((*it).at(0) == '/') // ignore slash in the beginning of path
					filename = (*it).substr(1);
				else
					filename = *it;
			}
			else
			{
				status_code = http_status_code::_404_NOT_FOUND_;
				return;
			}
			if (access(filename.c_str(), R_OK) < 0) // check file existence and read permission
			{
				perror("access");
				status_code = http_status_code::_404_NOT_FOUND_;
				return;
			}
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

			if (it++ != tokens.end() && *it == sprotocol)
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

			if (!(it++ != tokens.end() && *it == sprotocol))
			{
				status_code = http_status_code::_400_BAD_REQUEST_;
				return;
			}
		}
		bool typegiven = false;
		bool lengthgiven = false;
		while (getline(headeriss, line)) // parse other required fields
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
	ss << method_to_str(method) << " " << filename << " " << sprotocol << "\r\n";
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
	ss << sprotocol << " " << status_code_strings[status_code] << "\r\n";
	ss << "Content-Type: text/plain" << "\r\n";
	if (method == http_method::GET)
	{
		if (status_code == http_status_code::_200_OK_)
			ss << "Content-Length: " << content_length << "\r\n";
		else
			ss << "Content-Length: 0" << "\r\n";
	}
	else if (method == http_method::PUT)
		ss << "Content-Length: 0" << "\r\n";
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

http_request::http_request() : method(http_method::NOT_SET), content_length(0)
{ }

http_request http_request::from_socket(int sockfd)
{
	http_request request;

	/* strip header from the message */
	bool delimiterfound = false;
	int recvd;
	char buffer[RECVBUFSIZE];
	memset(buffer, 0, RECVBUFSIZE);
	std::string readtotal;
	size_t found; // index of start of delimiter
	std::string header;
	while (!delimiterfound && (recvd = read(sockfd, buffer, RECVBUFSIZE)) > 0)
	{
		std::string chunk(buffer, recvd);
		readtotal += chunk;
		found = readtotal.find(delimiter);
		if (found != std::string::npos)
		{
			delimiterfound = true;
			header = readtotal.substr(0, found + delimiter.length()); // include delimiter to header
		}
	}
	if (recvd < 0)
	{
		perror("read");
		return request;
	}
	if (!delimiterfound)
	{
		std::cerr << "delimiter not found" << std::endl;
		return request;
	}
	request.header = header;

	/* parse header fields from the header */
	request.parse_header();

	return request;
}

http_request http_request::from_params(std::string method, std::string filename, std::string hostname, std::string username)
{
	http_request request;

	std::transform(method.begin(), method.end(), method.begin(), ::toupper); // method to upper case
	switch (stringtomethod[method])
	{
	case http_method::GET:
		request.method = http_method::GET;
		break;
	case http_method::PUT:
		request.method = http_method::PUT;
		break;
	default:
		request.method = http_method::UNSUPPORTED;
		break;
	}

	request.filename = filename;
	request.protocol = sprotocol;
	request.hostname = hostname;
	request.username = username;

	request.create_header();

	return request;
}

void http_request::parse_header()
{
	using namespace std;
	istringstream headeriss(header);
	string line; // single line in header

	/* parse first line */
	if (!getline(headeriss, line))
		return;
	istringstream lineiss(line);
	vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
	if (tokens.size() == 0)
		return;
	vector<string>::const_iterator it = tokens.begin();
	if (*it == "GET")
		method = http_method::GET;
	else if (*it == "PUT")
		method = http_method::PUT;
	if (it++ != tokens.end())
	{
		if ((*it).length() > 0)
		{
			if ((*it).at(0) == '/') // ignore slash in the beginning of path
				filename = (*it).substr(1);
			else
				filename = *it;
		}
		else
			return;

		if (it++ != tokens.end())
			protocol = *it;
	}

	/* parse rest of lines */
	while (getline(headeriss, line))
	{
		istringstream lineiss(line);
		vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
		if (tokens.size() > 0)
		{
			vector<string>::const_iterator it = tokens.begin();
			if (*it == "Content-Type:")
			{
				if (it++ != tokens.end())
					content_type = *it;
			}
			else if (*it == "Content-Length:")
			{
				if (it++ != tokens.end())
				{
					istringstream iss(*it);
					iss >> content_length; // convert to size_t
				}
			}
		}
	}
}

void http_request::print() const
{
	std::cout << std::endl << "*** Request header ***" << std::endl
			  << header << std::endl
			  << "**********************" << std::endl << std::endl
			  << "*** Parsed values ***" << std::endl
			  << "Method: " << method_strings[method] << std::endl
			  << "Filename: " << filename << std::endl
			  << "Protocol: " << protocol << std::endl
			  << "Hostname: " << hostname << std::endl
			  << "Username: " << username << std::endl
			  << "Content-Type: " << content_type << std::endl
			  << "Content-Length: " << content_length << std::endl
			  << "*********************" << std::endl << std::endl;
}

void http_request::create_header()
{
	std::stringstream headerss;
	headerss << method_strings[method] << " " << filename << " " << sprotocol << "\r\n";
	headerss << "Host: " << hostname << "\r\n";
	headerss << "Iam: " << username << "\r\n";
	if (method == http_method::PUT)
	{
		headerss << "Content-Type: " << scontenttype << "\r\n";
		headerss << "Content-Length: " << content_length << "\r\n";
	}

	header = headerss.str();
}
