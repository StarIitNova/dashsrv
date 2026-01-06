#pragma once

#include <Server/Core/Server.h>

#include <Server/Client.h>

class ServiceHandler {
  public:
    ~ServiceHandler();

    static ServiceHandler &get() {
        static ServiceHandler handler;
        return handler;
    }

    ServiceHandler(const ServiceHandler &&) = delete;
    ServiceHandler operator=(const ServiceHandler &&) = delete;

    void init();

    void run();

  private:
    NoreServer *mServer = nullptr;
    std::unordered_map<std::string, UserClient> mClients;

    ServiceHandler() = default;

    void onWebsocketConnect(const std::string &id);
    void onWebsocketClose(const std::string &id);
    void onWebsocketMessage(const std::string &id, const std::string &data);
};
