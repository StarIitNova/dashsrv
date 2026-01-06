#include "Basic.h"
#include <Server/Routes.h>

#include <Server/Core/Routing.h>
#include <Server/CacheContainer.h>

#include <Minecraft/MCDef.h>
#include <Minecraft/Status.h>

#include <MGClient.h>
#include <Hardware.h>

#include <nlohmann/json.hpp>

#include <print>
#include <string>
#include <cmath>

struct JellyfinStatus {
    bool Online;
    std::string Health;

    std::string LocalAddress;
    std::string ServerName;
    std::string Version;
    std::string ProductName;
    std::string OperatingSystem;
    std::string ID;
    bool StartupWizardComplete;
};

struct DashboardStatus {
    std::vector<std::string> IPs;
    struct {
        uint64_t Available;
        uint64_t Total;
    } Memory;
    double CPU;
    uint64_t Ping;
    bool IsCurrent;
    bool Online;
};

struct DashboardHealthStatus {
    std::vector<DashboardStatus> Statuses;
};

static CacheContainer<Minecraft::MCStatus, 10000> ServerCache;
static CacheContainer<JellyfinStatus, 30000> JellyfinCache;
static CacheContainer<DashboardStatus, 5000> HardwareCache;
static CacheContainer<DashboardHealthStatus, 5000> MeshCache;

static double LastValidCPUUsage = 0.0;

static const Minecraft::MCServer MinecraftServer{ "192.168.0.105", 25565, 774 };
static const std::string MinecraftDomain = "copyright-postings.gl.joinmc.link";

static const std::string JellyfinIP = "192.168.1.47:8096";

static const std::vector<std::string> NetworkServerIPs = { "192.168.0.105", "192.168.1.47" };

JellyfinStatus GetJellyfinStatus();
DashboardStatus GetDashboardStatus();
DashboardHealthStatus GetHealthReport();

std::string MCStatusToJSON(const Minecraft::MCStatus &status, bool cached);
std::string JellyfinStatusToJSON(const JellyfinStatus &status, bool cached);
std::string DashboardStatusToJSON(const DashboardStatus &status, bool cached);
std::string HealthReportToJSON(const DashboardHealthStatus &status, bool cached);

bool handleRoutes(const RequestData &req, ResponseData &res) {
    res.status = 0;
    ROUTE("/api") {
        GET("/mc") {
            bool cached = true;
            if (ServerCache.NeedsFetch()) {
                Minecraft::MCStatus fetched = Minecraft::QueryServer(MinecraftServer);
                ServerCache.Cache(fetched);
                cached = false;
            }

            const Minecraft::MCStatus &status = ServerCache.Get();

            res.content_type = "application/json";
            res.body = MCStatusToJSON(status, cached);
            res.status = 200;
            res.handled = true;
        }

        GET("/jellyfin") {
            bool cached = true;
            if (JellyfinCache.NeedsFetch()) {
                JellyfinStatus fetched = GetJellyfinStatus();
                JellyfinCache.Cache(fetched);
                cached = false;
            }

            const JellyfinStatus &status = JellyfinCache.Get();

            res.content_type = "application/json";
            res.body = JellyfinStatusToJSON(status, cached);
            res.status = 200;
            res.handled = true;
        }

        GET("/status") {
            bool cached = true;
            if (MeshCache.NeedsFetch()) {
                DashboardHealthStatus health = GetHealthReport();
                MeshCache.Cache(health);
                cached = false;
            }

            DashboardHealthStatus health = MeshCache.Get();

            res.content_type = "application/json";
            res.body = HealthReportToJSON(health, cached);
            res.status = 200;
            res.handled = true;
        }

        GET("/local") {
            bool cached = true;
            if (HardwareCache.NeedsFetch()) {
                DashboardStatus status = GetDashboardStatus();
                HardwareCache.Cache(status);
                cached = false;
            }

            DashboardStatus status = HardwareCache.Get();
            
            res.content_type = "application/json";
            res.body = DashboardStatusToJSON(status, cached);
            res.status = 200;
            res.handled = true;
        }
    }

    GET("/") {
        res.respondFile("resources/static/pages/dashboard.html");
        res.headers["Content-Security-Policy"] = "default-src 'self'; img-src 'self' data:; style-src 'self' https://fonts.googleapis.com; font-src 'self' https://fonts.gstatic.com;";
    }

    GET("/favicon.ico") {
        res.respondFile("resources/static/favicon24x.png");
    }

    if (req.path.starts_with("/static")) {
        res.respondDirectory("resources/static/", "/static", req.url);
    }

    if (res.status == 404 || res.status == 0) {
        res.respondFile("resources/static/common/404.html");
        res.status = 404;
        res.handled = true;
    }

    return res.handled;
}

JellyfinStatus GetJellyfinStatus() {
    JellyfinStatus status;
    status.Online = false;

    do {
        MGResponse healthRes = Dashcli::Get(JellyfinIP + "/health");
        if (!healthRes.Success) {
            status.Online = false;
            break;
        }
                    
        std::string healthstr(healthRes.Recv.begin(), healthRes.Recv.end());
        status.Health = healthstr;

        MGResponse jsonRes = Dashcli::Get(JellyfinIP + "/System/Info/Public");
        if (!jsonRes.Success) {
            status.Online = false;
            break;
        }

        std::string_view sv(reinterpret_cast<const char *>(jsonRes.Recv.data()), jsonRes.Recv.size());
        nlohmann::json json = nlohmann::json::parse(sv);

        if (json.is_discarded()) {
            status.Online = false;
            break;
        }

        try {
            status.Online = true;
            status.LocalAddress = json["LocalAddress"];
            status.ServerName = json["ServerName"];
            status.Version = json["Version"];
            status.ProductName = json["ProductName"];
            status.OperatingSystem = json["OperatingSystem"];
            status.ID = json["Id"];
            status.StartupWizardComplete = json["StartupWizardCompleted"];
        } catch (const std::exception &e) {
            std::println("Caught exception while parsing jellyfin status JSON");
            status.Online = false;
            return status;
        }
    } while (false);

    return status;
}

DashboardStatus GetDashboardStatus() {
    DashboardStatus result;
    std::vector<std::string> ips = GetLocalIPs();
    MemoryInfo mem = GetMemoryUsage();
    
    double cpu = gCPUUsage.GetCPUUsage();
    while (!std::isfinite(cpu) && LastValidCPUUsage == 0.0) {
        cpu = gCPUUsage.GetCPUUsage();
        gCPUUsage.Tick();
        if (std::isfinite(cpu))
            LastValidCPUUsage = cpu;
    }

    result.CPU = cpu;
    result.IPs = ips;
    result.Memory.Available = mem.availableMB;
    result.Memory.Total = mem.totalMB;
    result.Ping = 0;
    result.IsCurrent = true;
    result.Online = true;

    return result;
}

DashboardHealthStatus GetHealthReport() {
    DashboardHealthStatus health;
    if (HardwareCache.NeedsFetch()) {
        HardwareCache.Cache(GetDashboardStatus());
    }
    
    DashboardStatus self = HardwareCache.Get();
    health.Statuses.push_back(self);
    
    for (const auto &server : NetworkServerIPs) {
        bool isSelf = false;
        for (const auto &ip : self.IPs) {
            if (ip == server) {
                isSelf = true;
                break;
            }
        }

        if (isSelf) continue;

        DashboardStatus status;
        status.IPs.push_back(server);

        uint64_t start = GetTimeMillis();
        MGResponse healthRes = Dashcli::Get(server + "/api/local");
        if (!healthRes.Success) {
            status.Online = false;
            health.Statuses.push_back(status);
            continue;
        }

        std::string_view sv(reinterpret_cast<const char *>(healthRes.Recv.data()), healthRes.Recv.size());
        nlohmann::json json = nlohmann::json::parse(sv);

        if (json.is_discarded()) {
            status.Online = false;
            health.Statuses.push_back(status);
            break;
        }

        try {
            status.Online = true;
            status.IsCurrent = false;
            status.CPU = json["cpu"];
            status.IPs = json["ips"];
            status.Memory.Available = json["memory"]["available"];
            status.Memory.Total = json["memory"]["total"];
            status.Ping = GetTimeMillis() - start;
            health.Statuses.push_back(status);
        } catch (const std::exception &e) {
            std::println("Caught exception while parsing server status JSON");
            status.Online = false;
            status.IPs.clear();
            status.IPs.push_back(server);
            health.Statuses.push_back(status);
            continue;
        }
    }

    return health;
}

std::string MCStatusToJSON(const Minecraft::MCStatus &status, bool cached) {
    try {
        nlohmann::json json;
        json["cached"] = cached;
        json["cacheTiming"] = ServerCache.GetTiming();
        json["online"] = status.Online;
        json["ip"] = MinecraftServer.IP;
        json["domain"] = MinecraftDomain;
        json["port"] = MinecraftServer.Port;
        json["requestProtocol"] = MinecraftServer.ProtocolVersion;
        if (status.Online) {
            json["version"]["name"] = status.Version.Name;
            json["version"]["protocol"] = status.Version.Protocol;
            json["motd"] = status.MOTD;
            json["ping"] = status.PingMS;
            json["players"]["online"] = status.Players.Online;
            json["players"]["max"] = status.Players.Max;
            json["icon"] = status.Icon;
        }

        return json.dump();
    } catch (const std::exception &e) {
        std::println("Caught exception while generating MC status JSON");
        return "{}";
    }
}

std::string JellyfinStatusToJSON(const JellyfinStatus &status, bool cached) {
    try {
        nlohmann::json json;
        json["cached"] = cached;
        json["cacheTiming"] = JellyfinCache.GetTiming();
        json["online"] = status.Online;
        json["healthString"] = status.Health;
        json["localAddress"] = status.LocalAddress;
        json["serverName"] = status.ServerName;
        json["version"] = status.Version;
        json["productName"] = status.ProductName;
        json["os"] = status.OperatingSystem.empty() ? "Unknown" : status.OperatingSystem;
        json["id"] = status.ID;
        json["startupWizardComplete"] = status.StartupWizardComplete;

        return json.dump();
    } catch (const std::exception &e) {
        std::println("Caught exception while generation Jellyfin status JSON");
        return "{}";
    }
}

std::string DashboardStatusToJSON(const DashboardStatus &status, bool cached) {
    try {
        nlohmann::json json;
        json["cached"] = cached;
        json["cacheTiming"] = HardwareCache.GetTiming();
        json["online"] = status.Online;
        json["ips"] = status.IPs;
        if (status.Online) {
            if (std::isfinite(status.CPU)) {
                json["cpu"] = status.CPU;
            } else {
                json["cpu"] = LastValidCPUUsage;
            }
            
            json["ping"] = status.Ping;
            json["memory"]["available"] = status.Memory.Available;
            json["memory"]["total"] = status.Memory.Total;
            json["memory"]["usage"] = (double)(status.Memory.Total - status.Memory.Available) / (double)status.Memory.Total;
            json["self"] = status.IsCurrent;
        }

        return json.dump();
    } catch (const std::exception &e) {
        std::println("Caught exception while generating dashboard status JSON");
        return "{}";
    }
}

std::string HealthReportToJSON(const DashboardHealthStatus &status, bool cached) {
    std::string data = "";
    for (size_t i = 0; i < status.Statuses.size(); ++i) {
        if (i != 0) data += ",";
        data += DashboardStatusToJSON(status.Statuses[i], cached);
    }
    
    return std::format(R"({{"cached":{},"cacheTiming":{},"data":[{}]}})", cached ? "true" : "false", MeshCache.GetTiming(), data);
}
