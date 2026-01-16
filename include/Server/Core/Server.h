#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include <mongoose.h>

enum class HttpMethod { HEAD, GET, DELETE_, POST, PUT, CONNECT, OPTIONS, TRACE, PATCH };

struct RequestData {
    std::string url;
    std::string scheme;
    std::string host;
    std::string subdomain;
    std::string domain;
    std::string port;
    std::string path;
    std::string query_string;
    std::unordered_map<std::string, std::string> query_params;

    HttpMethod method;

    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> cookies;

    std::string body;
    std::string content_type;
    size_t content_length = 0;
    std::string user_agent;

    std::string remote_address;
    uint16_t remote_port = 0;

    std::string http_version;
    bool is_secure = false;

    std::string getHeader(const std::string &key) const;
    std::string getQueryParam(const std::string &key) const;
    std::string getCookie(const std::string &key) const;
};

struct ResponseData {
    int status = 200;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> cookies;

    std::string body;
    std::string content_type = "text/plain";
    bool keep_alive = true;

    bool handled = false;

    void setHeader(const std::string &key, const std::string &value);
    void setCookie(const std::string &name, const std::string &value, const std::string &path = "/",
                   const std::string &extra = "");
    void setBody(const std::string &b, const std::string &type = "");

    void respondDirectory(const std::string &dir, const std::string &part, const std::string &url);
    void respondFile(const std::string &filepath);
};

class NoreServer {
  public:
    NoreServer(std::string address, std::function<bool(const RequestData &, ResponseData &)> handler);

    void run();
    bool onRequest(const RequestData &data, ResponseData &res);

    void attachWebsocketTools(std::function<void(const std::string &)> onConnection,
                              std::function<void(const std::string &, const std::string &)> onMessage,
                              std::function<void(const std::string &)> onClose);

    void sendToWebsocket(std::string id, const std::string &data);
    void broadcastWebsockets(const std::string &data);

  private:
    std::string mHostAddress;
    std::function<bool(const RequestData &, ResponseData &)> mHandlerFunction;
    std::function<void(const std::string &)> mWSConnect, mWSClose;
    std::function<void(const std::string &, const std::string &)> mWSMessage;

    std::unordered_map<struct mg_connection *, std::string> mWSIds;
    std::unordered_map<std::string, struct mg_connection *> mWSReverseLookup;

    friend void ev_handler(struct mg_connection *c, int ev, void *ev_data);
};

std::string constructResponseHeaders(const ResponseData &res);
