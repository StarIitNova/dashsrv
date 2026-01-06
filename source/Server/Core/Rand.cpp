#include <Server/Core/Rand.h>

#include <iomanip>
#include <random>
#include <sstream>
#include <string>

std::string generate_guid() {
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    uint64_t part1 = dis(gen);
    uint64_t part2 = dis(gen);

    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(8) << ((part1 >> 32) & 0xffffffff) << "-" << std::setw(4)
        << ((part1 >> 16) & 0xffff) << "-" << std::setw(4) << (part1 & 0xffff) << "-" << std::setw(4)
        << ((part2 >> 48) & 0xffff) << "-" << std::setw(12) << (part2 & 0xffffffffffff);
    return oss.str();
}
