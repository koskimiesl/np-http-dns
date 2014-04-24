#include "httpconf.hh"

http_conf::http_conf()
{
	init_maps();
}

void http_conf::init_maps()
{
	str_to_method = { {"NOT SET", http_method::NOT_SET},
					  {"GET", http_method::GET},
					  {"PUT", http_method::PUT},
					  {"POST", http_method::POST},
					  {"UNSUPPORTED", http_method::UNSUPPORTED} };

	method_to_str = { {http_method::NOT_SET, "NOT SET"},
					  {http_method::GET, "GET"},
					  {http_method::PUT, "PUT"},
					  {http_method::POST, "POST"},
					  {http_method::UNSUPPORTED, "UNSUPPORTED"} };

	str_to_status = { {"NOT SET", http_status::_NOT_SET_},
					  {"200 OK", http_status::_200_OK_},
					  {"201 Created", http_status::_201_CREATED_},
					  {"400 Bad Request", http_status::_400_BAD_REQUEST_},
					  {"403 Forbidden", http_status::_403_FORBIDDEN_},
					  {"404 Not Found", http_status::_404_NOT_FOUND_},
					  {"415 Unsupported Media Type", http_status::_415_UNSUPPORTED_MEDIA_TYPE_},
					  {"500 Internal Error", http_status::_500_INTERNAL_ERROR_},
					  {"501 Not Implemented", http_status::_501_NOT_IMPLEMENTED_},
					  {"UNSUPPORTED", http_status::_UNSUPPORTED_} };

	status_to_str = { {http_status::_NOT_SET_, "NOT SET"},
					  {http_status::_200_OK_, "200 OK"},
					  {http_status::_201_CREATED_, "201 Created"},
					  {http_status::_400_BAD_REQUEST_, "400 Bad Request"},
					  {http_status::_403_FORBIDDEN_, "403 Forbidden"},
					  {http_status::_404_NOT_FOUND_, "404 Not Found"},
					  {http_status::_415_UNSUPPORTED_MEDIA_TYPE_, "415 Unsupported Media Type"},
					  {http_status::_500_INTERNAL_ERROR_, "500 Internal Error"},
					  {http_status::_501_NOT_IMPLEMENTED_, "501 Not Implemented"},
					  {http_status::_UNSUPPORTED_, "UNSUPPORTED"} };
}
