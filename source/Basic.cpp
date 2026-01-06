#include <Basic.h>

#include <chrono>

uint64_t GetTimeMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock().now().time_since_epoch()).count();
}
