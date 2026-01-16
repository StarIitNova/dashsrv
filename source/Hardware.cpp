#include <Hardware.h>

#ifdef _WIN32
#define _WIN32_WINNT 0x0600
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <fstream>
#include <ifaddrs.h>
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach/host_info.h>
#include <mach/mach.h>
#endif

CPUUsage gCPUUsage;

std::vector<std::string> GetLocalIPs() {
    std::vector<std::string> ips;
#ifdef _WIN32
    ULONG size = 0;
    GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &size);
    IP_ADAPTER_ADDRESSES *adapters = (IP_ADAPTER_ADDRESSES *)malloc(size);
    if (GetAdaptersAddresses(AF_INET, 0, nullptr, adapters, &size) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES *adapter = adapters; adapter; adapter = adapter->Next) {
            for (IP_ADAPTER_UNICAST_ADDRESS *addr = adapter->FirstUnicastAddress; addr; addr = addr->Next) {
                SOCKADDR_IN *sa_in = (SOCKADDR_IN *)addr->Address.lpSockaddr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sa_in->sin_addr, ip, INET_ADDRSTRLEN);
                ips.push_back(ip);
            }
        }
    }
    free(adapters);
#else
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1)
        return ips;

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr)
            continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            char ip[INET_ADDRSTRLEN];
            void *addr_ptr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr_ptr, ip, INET_ADDRSTRLEN);
            ips.push_back(ip);
        }
    }
    freeifaddrs(ifaddr);
#endif
    return ips;
}

MemoryInfo GetMemoryUsage() {
    MemoryInfo mem{ 0, 0 };
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    mem.totalMB = status.ullTotalPhys / 1024 / 1024;
    mem.availableMB = status.ullAvailPhys / 1024 / 1024;
#elif defined(__APPLE__)
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vmstat;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vmstat, &count) != KERN_SUCCESS) {
        return mem;
    }

    vm_size_t page_size;
    host_page_size(mach_host_self(), &page_size);

    uint64_t free = vmstat.free_count * page_size;
    uint64_t inactive = vmstat.inactive_count * page_size;
    uint64_t wired = vmstat.wire_count * page_size;
    uint64_t active = vmstat.active_count * page_size;

    mem.totalMB = (free + inactive + active + wired) / 1024 / 1024;
    mem.availableMB = (free + inactive) / 1024 / 1024;
#else
    std::ifstream file("/proc/meminfo");
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") == 0)
            mem.totalMB = std::stoull(line.substr(9)) / 1024;
        if (line.find("MemAvailable:") == 0)
            mem.availableMB = std::stoull(line.substr(13)) / 1024;
    }
#endif
    return mem;
}

CPUUsage::CPUUsage() {
#ifdef _WIN32
    GetSystemTimes(&prevIdleTime, &prevKernelTime, &prevUserTime);
#elif defined(__APPLE__)
    ReadMacCPU(prevIdleTicks, prevTotalTicks);
#else
    ReadProcStat(prevIdleTimeLinux, prevTotalTimeLinux);
#endif
}

void CPUUsage::Tick() {
#ifdef _WIN32
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);

    ULONGLONG idle = FileTimeToULL(idleTime) - FileTimeToULL(prevIdleTime);
    ULONGLONG kernel = FileTimeToULL(kernelTime) - FileTimeToULL(prevKernelTime);
    ULONGLONG user = FileTimeToULL(userTime) - FileTimeToULL(prevUserTime);

    prevIdleTime = idleTime;
    prevKernelTime = kernelTime;
    prevUserTime = userTime;

    lastUsage = 100.0 * (double)(kernel + user - idle) / (kernel + user);
#elif defined(__APPLE__)
    unsigned long long idle, total;
    ReadMacCPU(idle, total);
    lastUsage = 100.0 * (double)(total - prevTotalTicks - (idle - prevIdleTicks)) / (total - prevTotalTicks);
    prevIdleTicks = idle;
    prevTotalTicks = total;
#else
    unsigned long long idle, total;
    ReadProcStat(idle, total);
    lastUsage =
        100.0 * (double)(total - prevTotalTimeLinux - (idle - prevIdleTimeLinux)) / (total - prevTotalTimeLinux);
    prevIdleTimeLinux = idle;
    prevTotalTimeLinux = total;
#endif
}

double CPUUsage::GetCPUUsage() const { return lastUsage; }

#ifdef _WIN32
ULONGLONG CPUUsage::FileTimeToULL(const FILETIME &ft) const {
    return (((ULONGLONG)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
}
#elif defined(__APPLE__)
unsigned long long prevIdleTicks = 0;
unsigned long long prevTotalTicks = 0;

void CPUUsage::ReadMacCPU(unsigned long long &idle, unsigned long long &total) {
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) != KERN_SUCCESS) {
        idle = total = 0;
        return;
    }
    idle = cpuinfo.cpu_ticks[CPU_STATE_IDLE];
    total = cpuinfo.cpu_ticks[CPU_STATE_USER] + cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] +
            cpuinfo.cpu_ticks[CPU_STATE_IDLE] + cpuinfo.cpu_ticks[CPU_STATE_NICE];
}
#else
void CPUUsage::ReadProcStat(unsigned long long &idle, unsigned long long &total) {
    std::ifstream file("/proc/stat");
    std::string line;
    getline(file, line);
    unsigned long long user, nice, system, iowait, irq, softirq, steal;
    sscanf(line.c_str(), "cpu %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait, &irq,
           &softirq, &steal);
    idle += iowait;
    total = user + nice + system + idle + irq + softirq + steal;
}
#endif
