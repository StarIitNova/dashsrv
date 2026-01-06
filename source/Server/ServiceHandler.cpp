#include <Server/ServiceHandler.h>

#include <Server/Routes.h>
#include <Basic.h>

#include <print>

ServiceHandler::~ServiceHandler() {
    if (mServer)
        delete mServer;
}

void ServiceHandler::init() {
    mServer = new NoreServer("http://0.0.0.0:80", handleRoutes);
}

void ServiceHandler::run() {
    if (!mServer) {
        return;
    }

    mServer->run();
}
