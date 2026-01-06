#ifndef DASHSRV_MCDEF_H__
#define DASHSRV_MCDEF_H__

#include <string>
#include <cstdint>

namespace Minecraft {

    struct MCServer {
        std::string IP;
        uint16_t Port;
        uint32_t ProtocolVersion;
    };

    struct MCStatus {
        struct {
            int Online;
            int Max;
        } Players;

        std::string MOTD;
        std::string Icon;

        struct {
            std::string Name;
            uint32_t Protocol;
        } Version;

        uint64_t PingMS;
        bool Online;

        std::string Error;
    };

}
    
#endif // DASHSRV_MCDEF_H__
