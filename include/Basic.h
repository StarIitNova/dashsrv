#ifndef DASHSRV_BASIC_H__
#define DASHSRV_BASIC_H__

#include <filesystem>
#include <functional>
#include <optional>
#include <stddef.h>
#include <stdint.h>
#include <string>

#define DASHSRV_VERSION "1.0"

#define UNUSED __attribute__((unused))

#define BIND_THIS(fn) std::bind(&fn, this, std::placeholders::_1)
#define BIND_THIS2(fn) std::bind(&fn, this, std::placeholders::_1, std::placeholders::_2)

uint64_t GetTimeMillis();

std::optional<std::string> ReadFile(const std::filesystem::path &filepath);
void WriteFile(const std::filesystem::path &filepath, const std::string &data);

std::string ResolveMDNS(const std::string &hostname, int timeoutMS = 1000);

#endif // DASHSRV_BASIC_H__
