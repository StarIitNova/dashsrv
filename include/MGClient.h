#ifndef DASHSRV_MGCLIENT_H__
#define DASHSRV_MGCLIENT_H__

#include <vector>
#include <cstdint>
#include <string>

struct MGResponse {
    std::string URL;

    std::vector<uint8_t> Sent;
    std::vector<uint8_t> Recv;

    bool Success = true;
    bool Done = false;
};

namespace Dashcli {

    MGResponse Get(std::string url);

}

#endif // DASHSRV_MGCLIENT_H__
