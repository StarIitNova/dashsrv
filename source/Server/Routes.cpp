#include <Server/Routes.h>

#include <Server/CacheContainer.h>
#include <Server/Config.h>
#include <Server/Core/Routing.h>

#include <Minecraft/MCDef.h>
#include <Minecraft/Status.h>

#include <Hardware.h>
#include <MGClient.h>

#include <nlohmann/json.hpp>

#include <cmath>
#include <iostream>
#include <string>

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

static const DashsrvConfig DashConfig("resources/config.json");
static DashsrvConfigServer *MinecraftInfo, *JellyfinInfo;

JellyfinStatus GetJellyfinStatus();
DashboardStatus GetDashboardStatus();
DashboardHealthStatus GetHealthReport();

std::string MCStatusToJSON(const Minecraft::MCStatus &status, bool cached);
std::string JellyfinStatusToJSON(const JellyfinStatus &status, bool cached);
std::string DashboardStatusToJSON(const DashboardStatus &status, bool cached);
std::string HealthReportToJSON(const DashboardHealthStatus &status, bool cached);

bool handleRoutes(const RequestData &req, ResponseData &res) {
    res.status = 0;

    static bool scannedConfig = false;
    if (!scannedConfig) {
        for (auto &server : DashConfig.servers) {
            if (server.type == "minecraft") {
                MinecraftInfo = const_cast<DashsrvConfigServer *>(&server);
            }

            if (server.type == "jellyfin") {
                JellyfinInfo = const_cast<DashsrvConfigServer *>(&server);
            }
        }

        scannedConfig = true;
    }

    ROUTE("/api") {
        if (MinecraftInfo != nullptr) {
            GET("/mc") {
                bool cached = true;
                if (ServerCache.NeedsFetch()) {
                    DashsrvConfigServer::Minecraft mci =
                        std::get<DashsrvConfigServer::Minecraft>(MinecraftInfo->server);
                    Minecraft::MCServer server{ mci.ip, (uint16_t)mci.port, (uint32_t)mci.version };
                    Minecraft::MCStatus fetched = Minecraft::QueryServer(server);
                    ServerCache.Cache(fetched);
                    cached = false;
                }

                const Minecraft::MCStatus &status = ServerCache.Get();

                res.content_type = "application/json";
                res.body = MCStatusToJSON(status, cached);
                res.status = 200;
                res.handled = true;
            }
        }

        if (JellyfinInfo != nullptr) {
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
        res.headers["Content-Security-Policy"] =
            "default-src 'self'; img-src 'self' data:; style-src 'self' https://fonts.googleapis.com; font-src 'self' "
            "https://fonts.gstatic.com;";
    }

    GET("/favicon.ico") { res.respondFile("resources/static/favicon24x.png"); }

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
    DashsrvConfigServer::Jellyfin jfi = std::get<DashsrvConfigServer::Jellyfin>(JellyfinInfo->server);
    std::string jellyfinIP = jfi.ip + ":" + std::to_string(jfi.port);

    JellyfinStatus status;
    status.Online = false;

    do {
        MGResponse healthRes = Dashcli::Get(jellyfinIP + "/health");
        if (!healthRes.Success) {
            status.Online = false;
            break;
        }

        std::string healthstr(healthRes.Recv.begin(), healthRes.Recv.end());
        status.Health = healthstr;

        MGResponse jsonRes = Dashcli::Get(jellyfinIP + "/System/Info/Public");
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
            std::cout << "Caught exception while parsing jellyfin status JSON\n";
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

    for (const auto &serverInfo : DashConfig.servers) {
        if (serverInfo.type != "dashboard")
            continue;

        DashsrvConfigServer::Dashboard dbi = std::get<DashsrvConfigServer::Dashboard>(serverInfo.server);
        std::string server = dbi.ip;
        if (dbi.port != 80)
            server += ":" + std::to_string(dbi.port);

        bool isSelf = false;
        for (const auto &ip : self.IPs) {
            if (ip == server) {
                isSelf = true;
                break;
            }
        }

        if (isSelf)
            continue;

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
    DashsrvConfigServer::Minecraft mci = std::get<DashsrvConfigServer::Minecraft>(MinecraftInfo->server);

    try {
        nlohmann::json json;
        json["cached"] = cached;
        json["cacheTiming"] = ServerCache.GetTiming();
        json["online"] = status.Online;
        json["ip"] = mci.ip;
        json["domain"] = mci.extraDomain;
        json["port"] = mci.port;
        json["requestProtocol"] = mci.version;
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
            json["memory"]["usage"] =
                (double)(status.Memory.Total - status.Memory.Available) / (double)status.Memory.Total;
            json["self"] = status.IsCurrent;
        }

        return json.dump();
    } catch (const std::exception &e) {
        return "{}";
    }
}

std::string HealthReportToJSON(const DashboardHealthStatus &status, bool cached) {
    std::string data = "";
    for (size_t i = 0; i < status.Statuses.size(); ++i) {
        if (i != 0)
            data += ",";
        data += DashboardStatusToJSON(status.Statuses[i], cached);
    }

    return std::format(R"({{"cached":{},"cacheTiming":{},"data":[{}]}})", cached ? "true" : "false",
                       MeshCache.GetTiming(), data);
}
