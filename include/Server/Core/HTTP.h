#include "Server.h"

HttpMethod parseHttpMethod(const std::string &method);

std::unordered_map<std::string, std::string> parseQueryParams(const std::string &query);
