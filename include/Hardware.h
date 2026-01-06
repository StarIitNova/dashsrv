#ifndef DASHSRV_HARDWARE_H__
#define DASHSRV_HARDWARE_H__

#include <vector>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#include <windows.h>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#else
#include <ifaddrs.h>
#include <arpa/inet.h>
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
    ULONGLONG FileTimeToULL(const FILETIME& ft) const;
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
