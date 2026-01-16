#ifndef DASHSRV_MGCLIENT_H__
#define DASHSRV_MGCLIENT_H__

#include <cstdint>
#include <string>
#include <vector>

struct MGResponse {
    std::string URL;

    std::vector<uint8_t> Sent;
    std::vector<uint8_t> Recv;

    bool Success = true;
    bool Done = false;

    std::string Reason;
};

namespace Dashcli {

MGResponse Get(std::string url);

}

#endif // DASHSRV_MGCLIENT_H__
