#ifndef DASHSRV_HARDWARE_H__
#define DASHSRV_HARDWARE_H__

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
// clang-format on
#else
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>
#endif

struct MemoryInfo {
    uint64_t totalMB;
    uint64_t availableMB;
};

std::vector<std::string> GetLocalIPs();

MemoryInfo GetMemoryUsage();

class CPUUsage {
  public:
    CPUUsage();
    void Tick();
    double GetCPUUsage() const;

  private:
#ifdef _WIN32
    FILETIME prevIdleTime, prevKernelTime, prevUserTime;
    ULONGLONG FileTimeToULL(const FILETIME &ft) const;
#elif defined(__APPLE__)
    unsigned long long prevIdleTicks = 0;
    unsigned long long prevTotalTicks = 0;

    void ReadMacCPU(unsigned long long &idle, unsigned long long &total);
#else
    unsigned long long prevIdleTimeLinux = 0;
    unsigned long long prevTotalTimeLinux = 0;

    void ReadProcStat(unsigned long long &idle, unsigned long long &total);
#endif

    double lastUsage = 0.0;
};

extern CPUUsage gCPUUsage;

#endif // DASHSRV_HARDWARE_H__
