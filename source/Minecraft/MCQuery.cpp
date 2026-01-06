#include "Minecraft/MCPacket.h"
#include <Minecraft/MCQuery.h>

#include <mongoose.h>

static void mc_ev_handler(mg_connection *c, int ev, __attribute__((unused)) void *ev_data) {
    Minecraft::MCQueryState *state = static_cast<Minecraft::MCQueryState *>(c->fn_data);

    switch (ev) {
    case MG_EV_CONNECT: {
        // if (*(int *)ev_data != 0) {
        //     state->Done = true;
        //     return;
        // }

        auto msg_sender = [&](std::vector<uint8_t> data) {
            mg_send(c, data.data(), data.size());
        };

        state->OnConnect(msg_sender);
        break;
    }
    case MG_EV_READ: {
        mg_iobuf *buf = &c->recv;
        state->Recv.insert(state->Recv.end(), buf->buf, buf->buf + buf->len);
        buf->len = 0;
        break;
    }
    case MG_EV_CLOSE: {
        if (!state->Recv.empty()) {
            state->Success = true;
        }
        state->Done = true;
        break;
    }
    }
}

namespace Minecraft {
    void QueryMinecraft(MCQueryState &state, const MCServer &server, std::function<void(MCSendBytes)> onConnect) {
        mg_log_set(MG_LL_ERROR);
        
        state.Server = &server;
        state.OnConnect = onConnect;

        mg_mgr mgr;
        mg_mgr_init(&mgr);

        std::string addr = "tcp://" + server.IP + ":" + std::to_string(server.Port);
        mg_connect(&mgr, addr.c_str(), mc_ev_handler, &state);

        uint64_t timeout = 3 * 1000; // 3 seconds
        uint64_t start = packetool::GetTimeMS();

        while (!state.Done && (packetool::GetTimeMS() - start < timeout)) {
            mg_mgr_poll(&mgr, 50);
        }

        mg_mgr_free(&mgr);
    }
}
