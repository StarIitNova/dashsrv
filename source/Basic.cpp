#include <Basic.h>

#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stddef.h>
#include <stdexcept>
#include <string>
#include <thread>

#include <mdns/mdns.h> // mjansson/mdns library

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#endif

uint64_t GetTimeMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock().now().time_since_epoch()).count();
}

std::optional<std::string> ReadFile(const std::filesystem::path &filepath) {
    if (!std::filesystem::exists(filepath)) {
        return std::nullopt;
    }

    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }

    auto filesize = std::filesystem::file_size(filepath);

    std::string content(filesize, '\0');
    file.read(&content[0], filesize);

    if (!file) {
        throw std::runtime_error("Error reading file: " + filepath.string());
    }

    file.close();

    return content;
}

void WriteFile(const std::filesystem::path &filepath, const std::string &data) {
    std::ofstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file for writing: " + filepath.string());
    }

    file.write(data.data(), data.size());

    if (!file) {
        throw std::runtime_error("Error writing to file: " + filepath.string());
    }

    file.close();
}

int mdns_callback(int /*sock*/, const struct sockaddr * /*from*/, size_t /*addrlen*/, mdns_entry_type_t /*entry*/,
                  uint16_t /*query_id*/, uint16_t rtype, uint16_t /*rclass*/, uint32_t /*ttl*/, const void *data,
                  size_t size, size_t /*name_offset*/, size_t /*name_length*/, size_t /*record_offset*/,
                  size_t /*record_length*/, void *user_data) {
    if (rtype != MDNS_RECORDTYPE_A || size < 4 || !user_data)
        return 0; // Not an A record

    const uint8_t *bytes = static_cast<const uint8_t *>(data) + size - 4;
    char ip_str[INET_ADDRSTRLEN] = { 0 };

#ifdef _WIN32
    struct in_addr addr;
    memcpy(&addr, bytes, sizeof(addr));
    InetNtop(AF_INET, &addr, ip_str, sizeof(ip_str));
#else
    struct in_addr addr;
    memcpy(&addr, bytes, sizeof(addr));
    inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));
#endif

    std::string *result = static_cast<std::string *>(user_data);
    *result = ip_str;

    return 0;
}

std::unordered_map<std::string, std::string> &CachedMDNSResolutions() {
    static std::unordered_map<std::string, std::string> cachedMDNSResolutions;
    return cachedMDNSResolutions;
}

// Cross-platform mDNS resolver
std::string ResolveMDNS(const std::string &hostname, int timeout_ms) {
    if (CachedMDNSResolutions().contains(hostname)) {
        return CachedMDNSResolutions()[hostname];
    }

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return "";
#endif

    struct sockaddr_in saddr{};
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(0);

    int sock = mdns_socket_open_ipv4(&saddr);
    if (sock < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return "";
    }

    // Send query for A record
    char buffer[512];
    if (mdns_query_send(sock, MDNS_RECORDTYPE_A, hostname.c_str(), strlen(hostname.c_str()), buffer, sizeof(buffer),
                        0) < 0) {
        mdns_socket_close(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return "";
    }

    std::string result;
    auto start = std::chrono::steady_clock::now();
    while (result.empty()) {
        mdns_query_recv(sock, buffer, sizeof(buffer), mdns_callback, &result, 0);

        if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(timeout_ms)) {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    mdns_socket_close(sock);
#ifdef _WIN32
    WSACleanup();
#endif

    if (!result.empty())
        CachedMDNSResolutions()[hostname] = result;
    return result;
}
