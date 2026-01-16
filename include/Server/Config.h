#ifndef DASHSRV_SERVER_CONFIG_H__
#define DASHSRV_SERVER_CONFIG_H__

#include <string>
#include <variant>
#include <vector>

struct DashsrvConfigServer {
    struct Minecraft {
        std::string ip;
        int port;
        int version;
        std::string extraDomain;
    };

    struct Jellyfin {
        std::string ip;
        int port;
    };

    struct Dashboard {
        std::string ip;
        int port;
    };

    std::string type;
    std::variant<Minecraft, Jellyfin, Dashboard> server;
};

class DashsrvConfig {
  public:
    std::string ip;
    int port;

    std::vector<DashsrvConfigServer> servers;

    DashsrvConfig(const std::string &path);
};

#endif // DASHSRV_SERVER_CONFIG_H__
