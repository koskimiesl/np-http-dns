#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "dns.hh"
#include "general.hh"
#include "http.hh"
#include "networking.hh"

#define RECVBUFSIZE 1024

static const std::string sprotocol = "HTTP/1.1"; // supported protocol
static const std::string sctypegetput = "text/plain"; // supported content type for GET and PUT
static const std::string sctypepost = "application/x-www-form-urlencoded"; // supported content type for POST

static const std::string spostresource = "/dns-query"; // supported POST resource
static const std::string delimiter = "\r\n\r\n";

static std::map<std::string, http_method> strtomethod = { {"NOT SET", http_method::NOT_SET},
														  {"GET", http_method::GET},
														  {"PUT", http_method::PUT},
														  {"POST", http_method::POST},
														  {"UNSUPPORTED", http_method::UNSUPPORTED} };

static std::map<http_method, std::string> methodtostr = { {http_method::NOT_SET, "NOT SET"},
														  {http_method::GET, "GET"},
														  {http_method::PUT, "PUT"},
														  {http_method::POST, "POST"},
														  {http_method::UNSUPPORTED, "UNSUPPORTED"} };

static std::map<http_status, std::string> statustostr = { {http_status::_NOT_SET_, "NOT SET"},
															   {http_status::_200_OK_, "200 OK"},
															   {http_status::_201_CREATED_, "201 Created"},
															   {http_status::_400_BAD_REQUEST_, "400 Bad Request"},
															   {http_status::_403_FORBIDDEN_, "403 Forbidden"},
															   {http_status::_404_NOT_FOUND_, "404 Not Found"},
															   {http_status::_415_UNSUPPORTED_MEDIA_TYPE_, "415 Unsupported Media Type"},
															   {http_status::_500_INTERNAL_ERROR_, "500 Internal Error"},
														 	   {http_status::_501_NOT_IMPLEMENTED_, "501 Not Implemented"},
															   {http_status::_UNSUPPORTED_, "UNSUPPORTED"} };

static std::map<std::string, http_status> strtostatus = { {"NOT SET", http_status::_NOT_SET_},
															   {"200 OK", http_status::_200_OK_},
															   {"201 Created", http_status::_201_CREATED_},
															   {"400 Bad Request", http_status::_400_BAD_REQUEST_},
															   {"403 Forbidden", http_status::_403_FORBIDDEN_},
															   {"404 Not Found", http_status::_404_NOT_FOUND_},
															   {"415 Unsupported Media Type", http_status::_415_UNSUPPORTED_MEDIA_TYPE_},
															   {"500 Internal Error", http_status::_500_INTERNAL_ERROR_},
														 	   {"501 Not Implemented", http_status::_501_NOT_IMPLEMENTED_},
															   {"UNSUPPORTED", http_status::_UNSUPPORTED_} };

http_request::http_request(const http_conf& conf) : method(http_method::NOT_SET), content_length(0)
{
	this->conf = conf;
}

http_request http_request::form_header(const http_conf& conf, std::string method, std::string dirpath, std::string filename,
									   std::string hostname, std::string username, std::string queryname)
{
	http_request request(conf);

	switch (strtomethod[method])
	{
	case http_method::GET:
		request.method = http_method::GET;
		request.filename = filename;
		break;
	case http_method::PUT:
		int filesize;
		request.method = http_method::PUT;
		if (check_file(dirpath + "/" + filename, file_permissions::READ) != file_status::OK)
			throw http_exception();
		if ((filesize = get_file_size(dirpath + "/" + filename)) < 0)
			throw http_exception();
		request.filename = filename;
		request.content_type = sctypegetput;
		request.content_length = filesize;
		break;
	case http_method::POST:
		request.method = http_method::POST;
		request.filename = spostresource;
		request.content_type = sctypepost;
		request.queryname = queryname;
		request.querytype = SQUERYTYPE;
		request.content_length = strlen(request.get_query_body().c_str()) + 1; // includes terminating null character
		break;
	default:
		request.method = http_method::UNSUPPORTED;
		break;
	}

	request.protocol = sprotocol;
	request.hostname = hostname;
	request.username = username;

	request.create_header();

	return request;
}

http_request http_request::receive_header(int sockfd, const http_conf& conf)
{
	http_request request(conf);
	std::string header;

	if (!read_header(sockfd, delimiter, header))
		return request;

	request.header = header;

	/* parse header fields from the header */
	request.parse_header();

	return request;
}

void http_request::print_header() const
{
	std::cout << std::endl << "*** Request header ***" << std::endl
			  << header << std::endl
			  << "**********************" << std::endl << std::endl
			  << "*** Request header values ***" << std::endl
			  << "Method: " << methodtostr[method] << std::endl
			  << "Filename: " << filename << std::endl
			  << "Protocol: " << protocol << std::endl
			  << "Hostname: " << hostname << std::endl
			  << "Username: " << username << std::endl
			  << "Content-Type: " << content_type << std::endl
			  << "Content-Length: " << content_length << std::endl
			  << "*****************************" << std::endl << std::endl;
}

bool http_request::send(int sockfd, std::string dirpath) const
{
	/* determine if message will continue after header */
	bool payloadfollows = method == http_method::PUT || method == http_method::POST;

	/* send header */
	if (!send_message(sockfd, header, false, 0, payloadfollows))
		return false;

	/* send payload if needed */
	if (payloadfollows)
	{
		if (method == http_method::PUT)
		{
			if (!send_text_file(sockfd, dirpath, filename, content_length))
				return false;
		}
		else if (method == http_method::POST)
		{
			if (!send_message(sockfd, get_query_body(), true, content_length, false))
				return false;
		}
	}
	return true;
}

void http_request::create_header()
{
	std::stringstream headerss;
	headerss << methodtostr[method] << " " << filename << " " << protocol << "\r\n";
	headerss << "Host: " << hostname << "\r\n";
	headerss << "Iam: " << username << "\r\n";
	if (method == http_method::PUT || method == http_method::POST)
	{
		headerss << "Content-Type: " << content_type << "\r\n";
		headerss << "Content-Length: " << content_length << "\r\n";
	}
	headerss << "\r\n";
	header = headerss.str();
}

std::string http_request::get_query_body() const
{
	return "Name=" + queryname + "&Type=" + querytype;
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
	else if (*it == "POST")
		method = http_method::POST;
	else
		method = http_method::UNSUPPORTED;
	if (it++ != tokens.end())
	{
		if ((*it).length() > 0)
		{
			if ((*it).at(0) != '/') // add slash in front of URI if it doesn't exist
				filename = "/" + *it;
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
			if (*it == "Host:")
			{
				if (it++ != tokens.end())
					hostname = *it;
			}
			else if (*it == "Iam:")
			{
				if (it++ != tokens.end())
					username = *it;
			}
			else if (*it == "Content-Type:")
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

http_response::http_response() : status_code(http_status::_NOT_SET_), content_length(0), request_method(http_method::NOT_SET)
{ }

http_response http_response::proc_req_and_form_header(int sockfd, http_request request, std::string servpath, std::string username)
{
	http_response response;
	response.request_method = request.method;
	response.request_filename = request.filename;
	response.protocol = sprotocol;
	std::string filepath = servpath + request.filename;

	file_status getfilestatus, putfilestatus;
	std::string qbody;

	switch (request.method)
	{
	case http_method::GET:
		getfilestatus = check_file(filepath, file_permissions::READ);

		switch (getfilestatus)
		{
		case file_status::OK:
			int filesize;
			if ((filesize = get_file_size(servpath + request.filename)) < 0)
				response.status_code = http_status::_500_INTERNAL_ERROR_;
			else
			{
				response.status_code = http_status::_200_OK_;
				response.content_type = sctypegetput;
				response.content_length = filesize;
			}
			break;
		case file_status::DOES_NOT_EXIST:
			response.status_code = http_status::_404_NOT_FOUND_;
			break;
		case file_status::ACCESS_FAILURE:
			response.status_code = http_status::_403_FORBIDDEN_;
			break;
		default:
			response.status_code = http_status::_500_INTERNAL_ERROR_;
			break;
		}

		break;
	case http_method::PUT:
		if (request.content_type != sctypegetput)
		{
			response.status_code = http_status::_415_UNSUPPORTED_MEDIA_TYPE_;
			break;
		}
		putfilestatus = check_file(servpath + request.filename, file_permissions::WRITE);

		switch (putfilestatus)
		{
		case file_status::DOES_NOT_EXIST:
			if (recv_text_file(sockfd, servpath, request.filename, request.content_length))
				response.status_code = http_status::_201_CREATED_;
			else
				response.status_code = http_status::_500_INTERNAL_ERROR_;
			break;
		case file_status::OK:
			if (recv_text_file(sockfd, servpath, request.filename, request.content_length))
				response.status_code = http_status::_200_OK_;
			else
				response.status_code = http_status::_500_INTERNAL_ERROR_;
			break;
		case file_status::ACCESS_FAILURE:
			response.status_code = http_status::_403_FORBIDDEN_;
			break;
		default:
			response.status_code = http_status::_500_INTERNAL_ERROR_;
			break;
		}

		break;
	case http_method::POST:
		if (request.filename != spostresource)
		{
			response.status_code = http_status::_404_NOT_FOUND_;
			break;
		}
		if (request.content_type != sctypepost)
		{
			response.status_code = http_status::_415_UNSUPPORTED_MEDIA_TYPE_;
			break;
		}

		/* read query body from socket */
		if (!recv_body(sockfd, request.content_length, qbody))
		{
			response.status_code = http_status::_500_INTERNAL_ERROR_;
			break;
		}

		/* parse required parameters from body */
		if (!response.parse_req_query_params(qbody))
		{
			response.status_code = http_status::_400_BAD_REQUEST_;
			break;
		}
		std::cout << "sending query (name: " << response.request_qname << ", type: " << response.request_qtype << ")" << std::endl;
		send_query(response.request_qname, response.request_qtype);
		break;
	default:
		response.status_code = http_status::_501_NOT_IMPLEMENTED_;
		break;
	}

	response.username = username;
	response.create_header();

	return response;
}

http_response http_response::receive(int sockfd, http_method reqmethod, std::string dirpath, std::string filename)
{
	http_response response;

	std::string header;
	if (!read_header(sockfd, delimiter, header))
		return response;

	response.header = header;

	/* parse header fields from the header */
	response.parse_header();

	/* determine if message will continue after header */
	bool payloadfollows = reqmethod == http_method::GET && response.status_code == http_status::_200_OK_;

	if (payloadfollows)
	{
		std::cout << "receiving payload..." << std::endl;
		recv_text_file(sockfd, dirpath, filename, response.content_length); // read payload
	}

	return response;
}

void http_response::print_header() const
{
	std::cout << std::endl << "*** Response header ***" << std::endl
			  << header << std::endl
			  << "***********************" << std::endl << std::endl
			  << "*** Response header values ***" << std::endl
			  << "Protocol: " << protocol << std::endl
			  << "Status code: " << statustostr[status_code] << std::endl
			  << "Username: " << username << std::endl
			  << "Content-Type: " << content_type << std::endl
			  << "Content-Length: " << content_length << std::endl
			  << "******************************" << std::endl << std::endl;
}

bool http_response::send(int sockfd, std::string servpath) const
{
	/* determine if message will continue after header */
	bool payloadfollows = request_method == http_method::GET && status_code == http_status::_200_OK_;

	/* send header */
	if (!send_message(sockfd, header, false, 0, payloadfollows))
		return false;

	/* send payload if needed */
	if (payloadfollows)
	{
		if (!send_text_file(sockfd, servpath, request_filename, content_length))
			return false;
	}
	return true;
}

void http_response::create_header()
{
	std::stringstream headerss;
	headerss << protocol << " " << statustostr[status_code] << "\r\n";
	headerss << "Iam: " << username << "\r\n";
	if (request_method == http_method::GET && status_code == http_status::_200_OK_)
	{
		headerss << "Content-Type: " << content_type << "\r\n";
		headerss << "Content-Length: " << content_length << "\r\n";
	}
	headerss << "\r\n";
	header = headerss.str();
}

void http_response::parse_header()
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
	protocol = *it;
	it++;
	string statuscode;
	while (it != tokens.end())
	{
		statuscode += *it;
		it++;
		if (it != tokens.end())
			statuscode += " ";
	}
	status_code = strtostatus[statuscode];

	/* parse rest of lines */
	while (getline(headeriss, line))
	{
		istringstream lineiss(line);
		vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
		if (tokens.size() > 0)
		{
			vector<string>::const_iterator it = tokens.begin();
			if (*it == "Iam:")
			{
				if (it++ != tokens.end())
					username = *it;
			}
			else if (*it == "Content-Type:")
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

bool http_response::parse_req_query_params(const std::string& querybody)
{
	/* split parameters into a vector */
	std::vector<std::string> params = split_string(querybody, '&');

	/* read key and value from parameter */
	std::vector<std::string>::const_iterator paramsit = params.begin();
	bool qname = false;
	bool qtype = false;
	while (paramsit != params.end())
	{
		std::vector<std::string> paramtokens = split_string(*paramsit, '=');
		std::vector<std::string>::const_iterator tokensit = paramtokens.begin();
		if (tokensit != paramtokens.end())
		{
			std::string key = to_upper(*tokensit);
			tokensit++;
			if (tokensit != paramtokens.end())
			{
				std::string value = *tokensit;
				if (key == "NAME")
				{
					std::cout << "parsed value for 'Name': " << value << std::endl;
					request_qname = value;
					qname = true;
				}
				else if (key == "TYPE")
				{
					std::cout << "parsed value for 'Type': " << value << std::endl;
					request_qtype = value;
					qtype = true;
				}
			}
		}
		paramsit++;
	}
	if (qname && qtype)
		return true;

	return false;
}

http_exception::http_exception() : std::runtime_error("http_exception")
{ }
