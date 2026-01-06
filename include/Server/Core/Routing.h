#pragma once

#include <string>
#include <vector>

inline std::vector<std::string> __route_prefix_stack;

inline std::string __route_prefix() {
    std::string path;
    for (auto &p : __route_prefix_stack)
        path += p;
    return path;
}

inline bool starts_with(const std::string &s, const std::string &prefix) {
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

#define ROUTE(prefix)                                                                                                  \
    if (starts_with(req.path, __route_prefix() + prefix))                                                              \
        for (__route_prefix_stack.push_back(prefix); !__route_prefix_stack.empty(); __route_prefix_stack.pop_back())

#define GET(__path) if (req.method == HttpMethod::GET && req.path == (__route_prefix() + __path))

#define POST(__path) if (req.method == HttpMethod::POST && req.path == (__route_prefix() + __path))

#define PUT(__path) if (req.method == HttpMethod::PUT && req.path == (__route_prefix() + __path))

#define PATCH(__path) if (req.method == HttpMethod::PATCH && req.path == (__route_prefix() + __path))

#define DELETE_(__path) if (req.method == HttpMethod::DELETE && req.path == (__route_prefix() + __path))
