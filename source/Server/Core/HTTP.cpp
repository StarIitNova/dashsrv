#include <Server/Core/HTTP.h>

#include <sstream>
#include <string>

HttpMethod parseHttpMethod(const std::string &method) {
    if (method == "GET")
        return HttpMethod::GET;
    if (method == "POST")
        return HttpMethod::POST;
    if (method == "PUT")
        return HttpMethod::PUT;
    if (method == "DELETE")
        return HttpMethod::DELETE;
    if (method == "PATCH")
        return HttpMethod::PATCH;
    if (method == "HEAD")
        return HttpMethod::HEAD;
    if (method == "OPTIONS")
        return HttpMethod::OPTIONS;
    if (method == "TRACE")
        return HttpMethod::TRACE;
    if (method == "CONNECT")
        return HttpMethod::CONNECT;
    return HttpMethod::GET; // default fallback
}

std::unordered_map<std::string, std::string> parseQueryParams(const std::string &query) {
    std::unordered_map<std::string, std::string> params;
    std::istringstream ss(query);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto pos = token.find('=');
        if (pos != std::string::npos) {
            std::string key = token.substr(0, pos);
            std::string value = token.substr(pos + 1);
            params[key] = value;
        } else {
            params[token] = "";
        }
    }
    return params;
}
