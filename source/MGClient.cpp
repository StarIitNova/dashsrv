#include <MGClient.h>

#include <Basic.h>

#include <mongoose.h>

#include <iostream>

static void mg_ev_handler(mg_connection *c, int ev, void *ev_data) {
    MGResponse *res = static_cast<MGResponse *>(c->fn_data);

    switch (ev) {
    case MG_EV_CONNECT: {
        struct mg_str host = mg_url_host(res->URL.c_str());
        mg_printf(c,
                  "GET %s HTTP/1.1\r\n"
                  "Host: %.*s\r\n"
                  "User-Agent: dashsrv/1.0.0\r\n"
                  "Accept: */*\r\n"
                  "\r\n",
                  mg_url_uri(res->URL.c_str()), (int)host.len, host.buf);
        break;
    }
    case MG_EV_HTTP_MSG: {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;
        res->Recv.insert(res->Recv.end(), hm->body.buf, hm->body.buf + hm->body.len);
        res->Success = true;
        res->Done = true;
        break;
    }
    }
}

namespace Dashcli {

MGResponse Get(std::string url) {
    mg_log_set(MG_LL_ERROR);

    if (!url.starts_with("http://") || !url.starts_with("https://")) {
        url = "http://" + url;
    }

    MGResponse res;
    res.URL = url;
    res.Success = false;

    mg_mgr mgr;
    mg_mgr_init(&mgr);

    mg_http_connect(&mgr, url.c_str(), mg_ev_handler, &res);

    uint64_t timeout = 3000; // 3000 ms timeout
    uint64_t start = GetTimeMillis();

    while (!res.Done && (GetTimeMillis() - start < timeout)) {
        mg_mgr_poll(&mgr, 50);
    }

    res.Reason = "Connection closed";
    if (res.Success == false && GetTimeMillis() - start >= timeout) {
        res.Reason = "Timed out";
    }

    mg_mgr_free(&mgr);

    return res;
}

} // namespace Dashcli
