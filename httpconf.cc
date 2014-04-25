#include "httpconf.hh"

http_conf::http_conf() : protocol(http_protocol::HTTP_1_1), ctypegetput("text/plain"),
						 ctypepost("application/x-www-form-urlencoded"), uripost("/dns-query"),
						 delimiter("\r\n\r\n")
{
	init_maps();
}

http_protocol http_conf::to_prot(std::string str) const
{
	std::map<std::string, http_protocol>::const_iterator it;
		if ((it = str_to_prot.find(str)) != str_to_prot.end())
			return it->second;
		return http_protocol::UNSUPP_PROT;
}

std::string http_conf::to_str(http_protocol prot) const
{
	std::map<http_protocol, std::string>::const_iterator it;
	if ((it = prot_to_str.find(prot)) != prot_to_str.end())
		return it->second;
	return "UNSUPPORTED";
}

http_method http_conf::to_method(std::string str) const
{
	std::map<std::string, http_method>::const_iterator it;
	if ((it = str_to_method.find(str)) != str_to_method.end())
		return it->second;
	return http_method::UNSUPP_MET;
}

std::string http_conf::to_str(http_method method) const
{
	std::map<http_method, std::string>::const_iterator it;
	if ((it = method_to_str.find(method)) != method_to_str.end())
		return it->second;
	return "UNSUPPORTED";
}

http_status http_conf::to_status(std::string str) const
{
	std::map<std::string, http_status>::const_iterator it;
	if ((it = str_to_status.find(str)) != str_to_status.end())
		return it->second;
	return http_status::UNSUPP_ST;
}

std::string http_conf::to_str(http_status status) const
{
	std::map<http_status, std::string>::const_iterator it;
	if ((it = status_to_str.find(status)) != status_to_str.end())
		return it->second;
	return "UNSUPPORTED";
}

http_hfield http_conf::to_hfield(std::string str) const
{
	std::map<std::string, http_hfield>::const_iterator it;
		if ((it = str_to_hfield.find(str)) != str_to_hfield.end())
			return it->second;
		return http_hfield::UNSUPP_HF;
}

std::string http_conf::to_str(http_hfield hfield) const
{
	std::map<http_hfield, std::string>::const_iterator it;
	if ((it = hfield_to_str.find(hfield)) != hfield_to_str.end())
		return it->second;
	return "UNSUPPORTED";
}

void http_conf::init_maps()
{
	prot_to_str = { {http_protocol::NOT_SET_PROT, "NOT SET"},
					{http_protocol::HTTP_1_1, "HTTP/1.1"},
					{http_protocol::UNSUPP_PROT, "UNSUPPORTED"} };

	str_to_prot = { {"NOT SET", http_protocol::NOT_SET_PROT},
					{"HTTP/1.1", http_protocol::HTTP_1_1},
					{"UNSUPPORTED", http_protocol::UNSUPP_PROT} };

	method_to_str = { {http_method::NOT_SET_MET, "NOT SET"},
					  {http_method::GET, "GET"},
					  {http_method::PUT, "PUT"},
					  {http_method::POST, "POST"},
					  {http_method::UNSUPP_MET, "UNSUPPORTED"} };

	str_to_method = { {"NOT SET", http_method::NOT_SET_MET},
					  {"GET", http_method::GET},
					  {"PUT", http_method::PUT},
					  {"POST", http_method::POST},
					  {"UNSUPPORTED", http_method::UNSUPP_MET} };

	status_to_str = { {http_status::NOT_SET_ST, "NOT SET"},
					  {http_status::OK_200, "200 OK"},
					  {http_status::CREATED_201, "201 Created"},
					  {http_status::BAD_REQUEST_400, "400 Bad Request"},
					  {http_status::FORBIDDEN_403, "403 Forbidden"},
					  {http_status::NOT_FOUND_404, "404 Not Found"},
					  {http_status::UNSUPPORTED_MEDIA_TYPE_415, "415 Unsupported Media Type"},
					  {http_status::INTERNAL_ERROR_500, "500 Internal Error"},
					  {http_status::NOT_IMPLEMENTED_501, "501 Not Implemented"},
					  {http_status::UNSUPP_ST, "UNSUPPORTED"} };

	str_to_status = { {"NOT SET", http_status::NOT_SET_ST},
					  {"200 OK", http_status::OK_200},
					  {"201 CREATED", http_status::CREATED_201},
					  {"400 BAD REQUEST", http_status::BAD_REQUEST_400},
					  {"403 FORBIDDEN", http_status::FORBIDDEN_403},
					  {"404 NOT FOUND", http_status::NOT_FOUND_404},
					  {"415 UNSUPPORTED MEDIA TYPE", http_status::UNSUPPORTED_MEDIA_TYPE_415},
					  {"500 INTERNAL ERROR", http_status::INTERNAL_ERROR_500},
					  {"501 NOT IMPLEMENTED", http_status::NOT_IMPLEMENTED_501},
					  {"UNSUPPORTED", http_status::UNSUPP_ST} };

	hfield_to_str = { {http_hfield::HOST, "Host:"},
					  {http_hfield::IAM, "Iam:"},
					  {http_hfield::CONTENT_TYPE, "Content-Type:"},
					  {http_hfield::CONTENT_LEN, "Content-Length:"},
					  {http_hfield::UNSUPP_HF, "UNSUPPORTED:"} };

	str_to_hfield = { {"HOST:", http_hfield::HOST},
					  {"IAM:", http_hfield::IAM},
					  {"CONTENT-TYPE:", http_hfield::CONTENT_TYPE},
					  {"CONTENT-LENGTH:", http_hfield::CONTENT_LEN},
					  {"UNSUPPORTED", http_hfield::UNSUPP_HF} };
}
