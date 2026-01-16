#include <Server/ServiceHandler.h>

#include <Basic.h>
#include <Server/Config.h>
#include <Server/Routes.h>

#include <iostream>

ServiceHandler::~ServiceHandler() {
    if (mServer)
        delete mServer;
}

void ServiceHandler::init() {
    DashsrvConfig config("resources/config.json");
    std::cout << "Loaded configuration 'resources/config.json':\n";
    std::cout << "  IP: " << config.ip << ":" << config.port << "\n";
    std::cout << "  Servers: " << config.servers.size() << "\n";
    for (const auto &server : config.servers) {
        std::string ip, port;
        if (server.type == "minecraft") {
            ip = std::get<DashsrvConfigServer::Minecraft>(server.server).ip;
            port = std::to_string(std::get<DashsrvConfigServer::Minecraft>(server.server).port);
        } else if (server.type == "jellyfin") {
            ip = std::get<DashsrvConfigServer::Jellyfin>(server.server).ip;
            port = std::to_string(std::get<DashsrvConfigServer::Jellyfin>(server.server).port);
        } else if (server.type == "dashboard") {
            ip = std::get<DashsrvConfigServer::Dashboard>(server.server).ip;
            port = std::to_string(std::get<DashsrvConfigServer::Dashboard>(server.server).port);
        }
        std::cout << "    " << server.type << " " << ip << ":" << port << "\n";
    }

    mServer = new NoreServer("http://" + config.ip + ":" + std::to_string(config.port), handleRoutes);
}

void ServiceHandler::run() {
    if (!mServer) {
        return;
    }

    mServer->run();
}
