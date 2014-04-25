#include "httpconf.hh"

http_conf::http_conf()
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

http_method http_conf::to_method(std::string str) const
{
	std::map<std::string, http_method>::const_iterator it;
	if ((it = str_to_method.find(str)) != str_to_method.end())
		return it->second;
	return http_method::UNSUPPORTED;
}

http_hfield http_conf::to_hfield(std::string str) const
{
	std::map<std::string, http_hfield>::const_iterator it;
		if ((it = str_to_hfield.find(str)) != str_to_hfield.end())
			return it->second;
		return http_protocol::UNSUPP_PROT;
}

void http_conf::init_maps()
{
	const std::string notset = "NOT SET", unsupported = "UNSUPPORTED";
	const std::string http11 = "HTTP/1.1";
	const std::string get = "GET", put = "PUT", post = "POST";
	const std::string host = "HOST:", iam = "IAM:", ctype = "CONTENT-TYPE:", clen = "CONTENT-LENGTH:";

	prot_to_str = { {http_protocol::NOT_SET_PROT, notset},
					{http_protocol::HTTP_1_1, http11},
					{http_protocol::UNSUPP_PROT, unsupported} };

	str_to_prot = { {http11, http_protocol::HTTP_1_1},
					{unsupported, http_protocol::UNSUPP_PROT} };

	method_to_str = { {http_method::NOT_SET, notset},
					  {http_method::GET, "GET"},
					  {http_method::PUT, "PUT"},
					  {http_method::POST, "POST"},
					  {http_method::UNSUPPORTED, unsupported} };

	str_to_method = { {notset, http_method::NOT_SET},
					{get, http_method::GET},
					{put, http_method::PUT},
					{post, http_method::POST},
					{unsupported, http_method::UNSUPPORTED} };

	status_to_str = { {http_status::_NOT_SET_, notset},
					  {http_status::_200_OK_, "200 OK"},
					  {http_status::_201_CREATED_, "201 Created"},
					  {http_status::_400_BAD_REQUEST_, "400 Bad Request"},
					  {http_status::_403_FORBIDDEN_, "403 Forbidden"},
					  {http_status::_404_NOT_FOUND_, "404 Not Found"},
					  {http_status::_415_UNSUPPORTED_MEDIA_TYPE_, "415 Unsupported Media Type"},
					  {http_status::_500_INTERNAL_ERROR_, "500 Internal Error"},
					  {http_status::_501_NOT_IMPLEMENTED_, "501 Not Implemented"},
					  {http_status::_UNSUPPORTED_, unsupported} };

	str_to_status = { {notset, http_status::_NOT_SET_},
					  {"200 OK", http_status::_200_OK_},
					  {"201 Created", http_status::_201_CREATED_},
					  {"400 Bad Request", http_status::_400_BAD_REQUEST_},
					  {"403 Forbidden", http_status::_403_FORBIDDEN_},
					  {"404 Not Found", http_status::_404_NOT_FOUND_},
					  {"415 Unsupported Media Type", http_status::_415_UNSUPPORTED_MEDIA_TYPE_},
					  {"500 Internal Error", http_status::_500_INTERNAL_ERROR_},
					  {"501 Not Implemented", http_status::_501_NOT_IMPLEMENTED_},
					  {unsupported, http_status::_UNSUPPORTED_} };

	str_to_hfield = { {host, http_hfield::HOST},
					  {iam, http_hfield::IAM},
					  {ctype, http_hfield::CONTENT_TYPE},
					  {clen, http_hfield::CONTENT_LEN} };
}
