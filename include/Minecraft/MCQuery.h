#ifndef DASHSRV_MCQUERY_H__
#define DASHSRV_MCQUERY_H__

#include "MCDef.h"

#include <vector>
#include <functional>
#include <cstdint>

namespace Minecraft {
    using MCSendBytes = std::function<void(std::vector<uint8_t>)>;

    struct MCQueryState {
        const MCServer *Server;
        std::function<void(MCSendBytes)> OnConnect;

        std::vector<uint8_t> Recv;
        bool Done = false;
        bool Success = false;
    };

    void QueryMinecraft(MCQueryState &state, const MCServer &server, std::function<void(MCSendBytes)> onConnect);
}

#endif // DASHSRV_MCQUERY_H__
