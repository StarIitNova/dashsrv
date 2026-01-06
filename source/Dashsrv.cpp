#include <Basic.h>
#include <Server/ServiceHandler.h>

#include <print>

int main(UNUSED int argc, UNUSED char **argv) {
    ServiceHandler &handler = ServiceHandler::get();
    handler.init();
    handler.run();
    return 0;
}
