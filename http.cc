#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <unistd.h>
#include <vector>

#include "dns.hh"
#include "general.hh"
#include "http.hh"
#include "networking.hh"

http_request::http_request(const http_conf& conf) : header(), method(http_method::NOT_SET_MET), uri(),
													protocol(http_protocol::NOT_SET_PROT), hostname(), username(),
													content_type(), content_length(0), queryname(), querytype(), conf(conf)
{ }

http_request http_request::form_header(const http_conf& conf, std::string method, std::string dirpath, std::string filename,
									   std::string hostname, std::string username, std::string queryname)
{
	http_request req(conf);
	req.method = req.conf.to_method(method);
	req.protocol = req.conf.protocol;

	switch (req.method)
	{
	case http_method::GET:
		req.uri = filename;
		break;
	case http_method::PUT:
		int filesize;
		if (check_file_status(dirpath + filename, file_permissions::READ) != file_status::OK)
			throw general_exception("failed to check file existence and access permissions");
		if ((filesize = check_file_size(dirpath + filename)) < 0)
			throw general_exception("failed to get file size");
		req.uri = filename;
		req.content_type = req.conf.ctypegetput;
		req.content_length = filesize;
		break;
	case http_method::POST:
		req.uri = req.conf.uripost;
		req.content_type = req.conf.ctypepost;
		req.queryname = queryname;
		req.querytype = SQUERYTYPE;
		req.content_length = strlen(req.get_query_body().c_str()) + 1; // includes terminating null character
		break;
	default:
		throw general_exception("unsupported HTTP method");
	}

	req.hostname = hostname;
	req.username = username;

	req.create_header();

	return req;
}

http_request http_request::receive_header(const http_conf& conf, int sockfd)
{
	http_request req(conf);
	std::string header;

	if (!read_header(sockfd, req.conf.delimiter, header))
		throw general_exception("failed to read request header from socket");

	req.header = header;

	/* parse header fields from the header */
	if (!req.parse_header())
		throw general_exception("failed to parse request header");

	return req;
}

void http_request::print_header() const
{
	std::cout << std::endl << "*** Request header ***" << std::endl
			  << header << std::endl
			  << "**********************" << std::endl << std::endl
			  << "*** Request header values ***" << std::endl
			  << "Method: " << conf.to_str(method) << std::endl
			  << "URI: " << uri << std::endl
			  << "Protocol: " << conf.to_str(protocol) << std::endl
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
			if (!send_text_file(sockfd, dirpath, uri, content_length))
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
	headerss << conf.to_str(method) << " " << uri << " " << conf.to_str(protocol) << "\r\n";
	headerss << conf.to_str(http_hfield::HOST) << " " << hostname << "\r\n";
	headerss << conf.to_str(http_hfield::IAM) << " " << username << "\r\n";
	if (method == http_method::PUT || method == http_method::POST)
	{
		headerss << conf.to_str(http_hfield::CONTENT_TYPE) << " " << content_type << "\r\n";
		headerss << conf.to_str(http_hfield::CONTENT_LEN) << " " << content_length << "\r\n";
	}
	headerss << "\r\n";
	header = headerss.str();
}

std::string http_request::get_query_body() const
{
	return "Name=" + queryname + "&Type=" + querytype;
}

bool http_request::parse_header()
{
	using namespace std;
	istringstream headeriss(header);
	string line; // single line in header

	/* parse first line */
	if (!getline(headeriss, line))
		return false;
	istringstream lineiss(line);
	vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
	vector<string>::const_iterator it = tokens.begin();
	if (it == tokens.end())
		return false;
	method = conf.to_method(to_upper(*it));
	if (it++ != tokens.end())
	{
		if ((*it).length() > 0)
		{
			if ((*it).at(0) == '/')
				uri = *it;
			else
				uri = "/" + *it; // add slash in front of URI if it doesn't exist
		}
		else
			return false;

		if (it++ != tokens.end())
			protocol = conf.to_prot(to_upper(*it));
		else
			return false;
	}
	else
		return false;

	/* parse rest of lines */
	while (getline(headeriss, line))
	{
		istringstream lineiss(line);
		vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
		if (tokens.size() > 0)
		{
			vector<string>::const_iterator itfield = tokens.begin(); // field name
			vector<string>::const_iterator itvalue = itfield + 1; // field value
			if (itvalue == tokens.end()) return false;

			istringstream valueiss(*itvalue); // for type conversions
			switch(conf.to_hfield(to_upper(*itfield))) // recognize field in case-insensitive fashion
			{
			case http_hfield::HOST:
				valueiss >> hostname;
				break;
			case http_hfield::IAM:
				valueiss >> username;
				break;
			case http_hfield::CONTENT_TYPE:
				valueiss >> content_type;
				break;
			case http_hfield::CONTENT_LEN:
				valueiss >> content_length;
				break;
			case http_hfield::UNSUPP_HF:
				break; // ignore unsupported field
			default:
				break;
			}
		}
	}

	return true;
}

http_response::http_response(const http_conf& conf) : header(), protocol(http_protocol::NOT_SET_PROT), status(http_status::NOT_SET_ST), username(),
													  content_type(), content_length(0), request_method(http_method::NOT_SET_MET),
													  request_uri(), request_qname(), request_qtype(), conf(conf)
{ }

http_response http_response::proc_req_form_header(const http_conf& conf, int sockfd, http_request req, std::string servpath, std::string username)
{
	http_response resp(conf);
	resp.protocol = resp.conf.protocol;
	resp.request_method = req.method;
	resp.request_uri = req.uri;
	std::string filepath = servpath + req.uri;

	file_status getfilestatus, putfilestatus;
	std::string qbody;

	switch (req.method)
	{
	case http_method::GET:
		getfilestatus = check_file_status(filepath, file_permissions::READ);

		switch (getfilestatus)
		{
		case file_status::OK:
			int filesize;
			if ((filesize = check_file_size(servpath + req.uri)) < 0)
				resp.status = http_status::INTERNAL_ERROR_500;
			else
			{
				resp.status = http_status::OK_200;
				resp.content_type = resp.conf.ctypegetput;
				resp.content_length = filesize;
			}
			break;
		case file_status::DOES_NOT_EXIST:
			resp.status = http_status::NOT_FOUND_404;
			break;
		case file_status::ACCESS_FAILURE:
			resp.status = http_status::FORBIDDEN_403;
			break;
		default:
			resp.status = http_status::INTERNAL_ERROR_500;
			break;
		}

		break;
	case http_method::PUT:
		if (req.content_type != resp.conf.ctypegetput)
		{
			resp.status = http_status::UNSUPPORTED_MEDIA_TYPE_415;
			break;
		}
		putfilestatus = check_file_status(servpath + req.uri, file_permissions::WRITE);

		switch (putfilestatus)
		{
		case file_status::DOES_NOT_EXIST:
			if (recv_text_file(sockfd, servpath, req.uri, req.content_length))
				resp.status = http_status::CREATED_201;
			else
				resp.status = http_status::INTERNAL_ERROR_500;
			break;
		case file_status::OK:
			if (recv_text_file(sockfd, servpath, req.uri, req.content_length))
				resp.status = http_status::OK_200;
			else
				resp.status = http_status::INTERNAL_ERROR_500;
			break;
		case file_status::ACCESS_FAILURE:
			resp.status = http_status::FORBIDDEN_403;
			break;
		default:
			resp.status = http_status::INTERNAL_ERROR_500;
			break;
		}

		break;
	case http_method::POST:
		if (req.uri != resp.conf.uripost)
		{
			resp.status = http_status::NOT_FOUND_404;
			break;
		}
		if (req.content_type != resp.conf.ctypepost)
		{
			resp.status = http_status::UNSUPPORTED_MEDIA_TYPE_415;
			break;
		}

		/* read query body from socket */
		if (!recv_body(sockfd, req.content_length, qbody))
		{
			resp.status = http_status::INTERNAL_ERROR_500;
			break;
		}

		/* parse required parameters from body */
		if (!resp.parse_req_query_params(qbody))
		{
			resp.status = http_status::BAD_REQUEST_400;
			break;
		}

		std::cout << "doing DNS query with parameters: name: " << resp.request_qname << ", type: " << resp.request_qtype << std::endl;
		do_dns_query(resp.request_qname, resp.request_qtype);
		break;
	default:
		resp.status = http_status::NOT_IMPLEMENTED_501;
		break;
	}

	resp.username = username;
	resp.create_header();

	return resp;
}

http_response http_response::receive(const http_conf& conf, int sockfd, http_method reqmethod, std::string dirpath, std::string filename)
{
	http_response resp(conf);

	std::string header;
	if (!read_header(sockfd, resp.conf.delimiter, header))
		throw general_exception("failed to read response header from socket");

	resp.header = header;

	/* parse header fields from the header */
	if (!resp.parse_header())
		throw general_exception("failed to parse response header");

	/* determine if message will continue after header */
	bool payloadfollows = reqmethod == http_method::GET && resp.status == http_status::OK_200;

	if (payloadfollows)
	{
		std::cout << "receiving payload...";
		recv_text_file(sockfd, dirpath, filename, resp.content_length); // read payload
	}

	return resp;
}

http_response http_response::form_404_header(const http_conf& conf, std::string username)
{
	http_response resp(conf);
	resp.protocol = resp.conf.protocol;
	resp.status = http_status::NOT_FOUND_404;
	resp.username = username;
	resp.create_header();
	return resp;
}

void http_response::print_header() const
{
	std::cout << std::endl << "*** Response header ***" << std::endl
			  << header << std::endl
			  << "***********************" << std::endl << std::endl
			  << "*** Response header values ***" << std::endl
			  << "Protocol: " << conf.to_str(protocol) << std::endl
			  << "Status code: " << conf.to_str(status) << std::endl
			  << "Username: " << username << std::endl
			  << "Content-Type: " << content_type << std::endl
			  << "Content-Length: " << content_length << std::endl
			  << "******************************" << std::endl << std::endl;
}

bool http_response::send(int sockfd, std::string servpath) const
{
	/* determine if message will continue after header */
	bool payloadfollows = request_method == http_method::GET && status == http_status::OK_200;

	/* send header */
	if (!send_message(sockfd, header, false, 0, payloadfollows))
		return false;

	/* send payload if needed */
	if (payloadfollows)
	{
		std::cout << "sending payload...";
		if (!send_text_file(sockfd, servpath, request_uri, content_length))
			return false;
	}
	return true;
}

void http_response::create_header()
{
	std::stringstream headerss;
	headerss << conf.to_str(protocol) << " " << conf.to_str(status) << "\r\n";
	headerss << conf.to_str(http_hfield::IAM) << " " << username << "\r\n";
	if (request_method == http_method::GET && status == http_status::OK_200)
	{
		headerss << conf.to_str(http_hfield::CONTENT_TYPE) << " " << content_type << "\r\n";
		headerss << conf.to_str(http_hfield::CONTENT_LEN) << " " << content_length << "\r\n";
	}
	headerss << "\r\n";
	header = headerss.str();
}

bool http_response::parse_header()
{
	using namespace std;
	istringstream headeriss(header);
	string line; // single line in header

	/* parse first line */
	if (!getline(headeriss, line))
		return false;
	istringstream lineiss(line);
	vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
	if (tokens.size() == 0)
		return false;
	vector<string>::const_iterator it = tokens.begin();
	protocol = conf.to_prot(*it);
	it++;
	string statuscode;
	while (it != tokens.end())
	{
		statuscode += *it;
		it++;
		if (it != tokens.end())
			statuscode += " ";
	}
	status = conf.to_status(to_upper(statuscode)); // recognize status in case-insensitive fashion

	/* parse rest of lines */
	while (getline(headeriss, line))
	{
		istringstream lineiss(line);
		vector<string> tokens{istream_iterator<string>{lineiss}, istream_iterator<string>{}};
		if (tokens.size() > 0)
		{
			vector<string>::const_iterator itfield = tokens.begin();
			vector<string>::const_iterator itvalue = itfield + 1;
			if (itvalue == tokens.end()) return false;

			istringstream valueiss(*itvalue); // for type conversions
			switch (conf.to_hfield(to_upper(*itfield))) // recognize field in case-insensitive fashion
			{
			case http_hfield::IAM:
				valueiss >> username;
				break;
			case http_hfield::CONTENT_TYPE:
				valueiss >> content_type;
				break;
			case http_hfield::CONTENT_LEN:
				valueiss >> content_length;
				break;
			case http_hfield::UNSUPP_HF:
				break; // ignore unsuppported field
			default:
				break; // ignore unnecessary field
			}
		}
	}

	return true;
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
