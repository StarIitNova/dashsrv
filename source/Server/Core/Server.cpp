#include <Server/Core/Server.h>

#include <Server/Core/HTTP.h>
#include <Server/Core/Rand.h>

#include <Basic.h>
#include <Hardware.h>

#include <mongoose.h>

#include <filesystem>
#include <fstream>
#include <print>
#include <sstream>

namespace fs = std::filesystem;

std::string getMGEventString(int ev);

void mg_http_reply_nolen(struct mg_connection *c, int code, const std::string &headers, const void *body,
                         size_t body_len) {
    std::string resp =
        "HTTP/1.1 " + std::to_string(code) + " " + mg_http_status_code_str(code) + "\r\n" + headers + "\r\n";
    mg_send(c, resp.data(), resp.size());
    if (body_len > 0 && body != nullptr) {
        mg_send(c, body, body_len);
    }
    c->is_resp = 0;
}

void ev_handler(struct mg_connection *c, int ev, void *ev_data) {
    NoreServer *server = (NoreServer *)c->fn_data;

    if (ev != MG_EV_POLL) {
        // std::println("Catching request {}", getMGEventString(ev));
    }

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        RequestData req;
        std::string method(hm->method.buf, hm->method.buf + hm->method.len);
        req.method = parseHttpMethod(method);

        req.url = std::string(hm->uri.buf, hm->uri.buf + hm->uri.len);
        req.path = req.url;
        if (hm->query.len > 0) {
            req.query_string.assign(hm->query.buf, hm->query.len);
            req.query_params = parseQueryParams(req.query_string);
        }

        if (req.url == "/ws") {
            std::println("Upgrade requested");
            mg_ws_upgrade(c, hm, NULL);
            return;
        }

        for (size_t i = 0; i < MG_MAX_HTTP_HEADERS && hm->headers[i].name.len > 0; i++) {
            std::string name(hm->headers[i].name.buf, hm->headers[i].name.buf + hm->headers[i].name.len);
            std::string value(hm->headers[i].value.buf, hm->headers[i].value.buf + hm->headers[i].value.len);
            req.headers[name] = value;
        }

        if (hm->body.len > 0) {
            req.body.assign(hm->body.buf, hm->body.buf + hm->body.len);
            req.content_length = hm->body.len;
        }

        if (auto ct = mg_http_get_header(hm, "Content-Type")) {
            req.content_type.assign(ct->buf, ct->len);
        }
        if (auto ua = mg_http_get_header(hm, "User-Agent")) {
            req.user_agent.assign(ua->buf, ua->len);
        }
        if (auto cookie = mg_http_get_header(hm, "Cookie")) {
            std::string cookiestr(cookie->buf, cookie->buf + cookie->len);

            std::istringstream cs(cookiestr);
            std::string kv;
            while (std::getline(cs, kv, ';')) {
                auto pos = kv.find('=');
                if (pos != std::string::npos) {
                    std::string key = kv.substr(0, pos);
                    std::string value = kv.substr(pos + 1);
                    req.cookies[key] = value;
                }
            }
        }

        char addr[64];
        mg_snprintf(addr, sizeof(addr), "%M", mg_print_ip, &c->rem);
        req.remote_address = addr;
        req.remote_port = c->rem.port;

        req.scheme = c->is_tls ? "https" : "http";
        req.is_secure = c->is_tls;
        req.http_version = std::string(hm->proto.buf, hm->proto.buf + hm->proto.len);
        if (auto host = mg_http_get_header(hm, "Host")) {
            req.host.assign(host->buf, host->len);
        }

        if (!req.host.empty()) {
            auto dot = req.host.find('.');
            if (dot != std::string::npos && dot < req.host.size() - 1) {
                req.subdomain = req.host.substr(0, dot);
                req.domain = req.host.substr(dot + 1);
            }
        }

        // std::println("    Receiving request {} {} from {}", method, req.path, req.remote_address);

        ResponseData res;
        server->onRequest(req, res);

        if (!res.handled) {
            mg_http_reply(c, 404, "Content-Type: text/plain\r\n", "404 Not Found\n", method.c_str(), req.url.c_str(),
                          req.remote_address.c_str());
            return;
        }

        res.headers["Content-Length"] = std::to_string(res.body.size());
        std::string headers = constructResponseHeaders(res);

        mg_http_reply_nolen(c, res.status, headers, res.body.c_str(), res.body.size());
    } else if (ev == MG_EV_WS_OPEN) {
        std::string id = generate_guid();
        server->mWSIds[c] = id;
        server->mWSReverseLookup[id] = c;

        std::println("Websocket connection opened for client {}", id);

        if (server->mWSConnect) {
            server->mWSConnect(id);
        }
    } else if (ev == MG_EV_WS_MSG) {
        struct mg_ws_message *msg = (struct mg_ws_message *)ev_data;
        std::string msgstr(msg->data.buf, msg->data.buf + msg->data.len);

        std::string id = server->mWSIds[c];
        if (server->mWSMessage) {
            server->mWSMessage(id, msgstr);
        }

    } else if (ev == MG_EV_CLOSE) {
        if (c->is_websocket) {
            std::string id = server->mWSIds[c];
            server->mWSIds.erase(c);
            server->mWSReverseLookup.erase(id);

            std::println("Websocket connection {} closed", id);

            if (server->mWSClose) {
                server->mWSClose(id);
            }
        }
    }
}

NoreServer::NoreServer(std::string address, std::function<bool(const RequestData &, ResponseData &)> handler) {
    mHostAddress = address;
    mHandlerFunction = handler;
}

bool NoreServer::onRequest(const RequestData &req, ResponseData &res) { return mHandlerFunction(req, res); }

void NoreServer::run() {
    mg_log_set(MG_LL_ERROR); // disable most mongoose logging

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);
    struct mg_connection *c = mg_http_listen(&mgr, mHostAddress.c_str(), ev_handler, this);

    if (mHostAddress.starts_with("https://") || mHostAddress.starts_with("wss://")) {
        struct mg_tls_opts opts = { .cert = { (char *)"resources/server.crt", 20 },
                                    .key = { (char *)"resources/server.key", 20 } };
        mg_tls_init(c, &opts);
    }

    std::println("\nServer running on {}\n", mHostAddress);
    for (;;) {
        gCPUUsage.Tick();
        mg_mgr_poll(&mgr, 1000);
    }
}

void NoreServer::attachWebsocketTools(std::function<void(const std::string &)> onConnection,
                                      std::function<void(const std::string &, const std::string &)> onMessage,
                                      std::function<void(const std::string &)> onClose) {
    mWSConnect = onConnection;
    mWSMessage = onMessage;
    mWSClose = onClose;
}

void NoreServer::sendToWebsocket(std::string id, const std::string &data) {
    if (!mWSReverseLookup.contains(id))
        return;

    struct mg_connection *c = mWSReverseLookup[id];
    mg_ws_send(c, data.data(), data.size(), WEBSOCKET_OP_BINARY);
}

void NoreServer::broadcastWebsockets(const std::string &data) {
    for (const auto &[id, socket] : mWSReverseLookup) {
        mg_ws_send(socket, data.data(), data.size(), WEBSOCKET_OP_BINARY);
    }
}

std::string RequestData::getHeader(const std::string &key) const {
    return headers.contains(key) ? headers.at(key) : "";
}

std::string RequestData::getQueryParam(const std::string &key) const {
    return query_params.contains(key) ? query_params.at(key) : "";
}

std::string RequestData::getCookie(const std::string &key) const {
    return cookies.contains(key) ? cookies.at(key) : "";
}

void ResponseData::setHeader(const std::string &key, const std::string &value) { headers[key] = value; }

void ResponseData::setCookie(const std::string &name, const std::string &value, const std::string &path,
                             const std::string &extra) {
    std::string cookie = name + "=" + value + "; Path=" + path;
    if (!extra.empty())
        cookie += "; " + extra;
    cookies[name] = cookie;
}

void ResponseData::setBody(const std::string &b, const std::string &type) {
    body = b;
    if (!type.empty())
        content_type = type;
}

void ResponseData::respondDirectory(const std::string &dir, const std::string &part, const std::string &url) {
    if (url.rfind(part, 0) != 0) {
        status = 404;
        setBody("404 Not Found", "text/plain");
        handled = true;
        return;
    }

    std::string relative = url.substr(part.size());

    if (relative.empty() || relative == "/") {
        relative = "/index.html";
    }

    if (relative.find("..") != std::string::npos) {
        status = 403;
        setBody("403 Forbidden", "text/plain");
        handled = true;
        return;
    }

    fs::path fullPath = fs::path(dir) / relative.substr(1);

    if (fs::is_directory(fullPath)) {
        fullPath /= "index.html";
    }

    if (!fs::exists(fullPath) || !fs::is_regular_file(fullPath)) {
        status = 404;
        setBody("404 Not Found", "text/plain");
        handled = true;
        return;
    }

    respondFile(fullPath.string());
}

void ResponseData::respondFile(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        status = 404;
        setBody("404 Not Found", "text/plain");
        handled = true;
        return;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    body = ss.str();

    file.close();

    if (filepath.ends_with(".html"))
        content_type = "text/html";
    else if (filepath.ends_with(".css"))
        content_type = "text/css";
    else if (filepath.ends_with(".js"))
        content_type = "application/javascript";
    else if (filepath.ends_with(".png"))
        content_type = "image/png";
    else if (filepath.ends_with(".jpg") || filepath.ends_with(".jpeg"))
        content_type = "image/jpeg";
    else if (filepath.ends_with(".txt"))
        content_type = "text/plain";
    else
        content_type = "application/octet-stream";

    status = 200;
    handled = true;
}

std::string getHttpDate() {
    std::time_t now = std::time(nullptr);
    char buf[128];
    std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
    return buf;
}

std::string constructResponseHeaders(const ResponseData &res) {
    std::stringstream ss;

    ss << "Content-Type: " << (!res.content_type.empty() ? res.content_type : "text/plain; charset=utf-8") << "\r\n";
    ss << "Connection: " << (res.keep_alive ? "keep-alive" : "close") << "\r\n";
    ss << "Server: NoreServer/" DASHSRV_VERSION "\r\n";
    ss << "Date: " << getHttpDate() << "\r\n";

    std::unordered_map<std::string, std::string> headers = res.headers;
    if (!headers.contains("Cache-Control")) {
        if (res.content_type.starts_with("text/")) { // static endpoint
            headers["Cache-Control"] = "public, max-age=86400";
        } else {
            headers["Cache-Control"] = "no-cache, no-store, must-revalidate";
        }
    }

    if (!headers.contains("Content-Security-Policy")) {
        headers["Content-Security-Policy"] = "default-src 'self'";
    }

    if (!headers.contains("Referrer-Policy")) {
        headers["Referrer-Policy"] = "no-referrer";
    }

    if (!headers.contains("X-Frame-Options")) {
        headers["X-Frame-Options"] = "SAMEORIGIN";
    }

    for (const auto &[header, value] : headers) {
        ss << header << ": " << value << "\r\n";
    }

    return ss.str();
}

std::string getMGEventString(int ev) {
    switch (ev) {
    case MG_EV_ERROR:
        return "Error";
    case MG_EV_OPEN:
        return "Conn Open";
    case MG_EV_POLL:
        return "Conn Polled";
    case MG_EV_RESOLVE:
        return "Resolved Hostname";
    case MG_EV_CONNECT:
        return "Conn Established";
    case MG_EV_ACCEPT:
        return "Conn Accepted";
    case MG_EV_TLS_HS:
        return "TLS Handshake";
    case MG_EV_READ:
        return "Socket Data Read";
    case MG_EV_WRITE:
        return "Socket Data Write";
    case MG_EV_CLOSE:
        return "Conn Closed";
    case MG_EV_HTTP_HDRS:
        return "HTTP Headers";
    case MG_EV_HTTP_MSG:
        return "HTTP Message";
    case MG_EV_WS_OPEN:
        return "Websocket Handshake";
    case MG_EV_WS_MSG:
        return "Websocket Message";
    case MG_EV_WS_CTL:
        return "Websocket Control";
    case MG_EV_MQTT_CMD:
        return "MQTT Command";
    case MG_EV_MQTT_MSG:
        return "MQTT Message";
    case MG_EV_MQTT_OPEN:
        return "MQTT Open";
    case MG_EV_SNTP_TIME:
        return "SNTP Time";
    case MG_EV_WAKEUP:
        return "mg_wakeup()";
    case MG_EV_USER:
        return "User Event ID";
    default:
        return "Unknown";
    }
}
