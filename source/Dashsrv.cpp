#include <Basic.h>
#include <Server/ServiceHandler.h>

int main(UNUSED int argc, UNUSED char **argv) {
    ServiceHandler &handler = ServiceHandler::get();
    handler.init();
    handler.run();
    return 0;
}
