#include <Server/Config.h>

#include <Basic.h>

#include <nlohmann/json.hpp>

#include <stdexcept>

static constexpr const char *DefaultConfig = "{\n    \"hostip\": \"0.0.0.0\",\n    \"hostport\": 8080\n}";

DashsrvConfig::DashsrvConfig(const std::string &path) {
    auto configOpt = ReadFile(path);

    std::string configStr;
    if (!configOpt) {
        WriteFile(path, DefaultConfig);
        configStr = DefaultConfig;
    } else {
        configStr = *configOpt;
    }

    nlohmann::json json = nlohmann::json::parse(configStr);

    if (!json.is_discarded()) {
        if (json.contains("hostip")) {
            ip = json["hostip"];
            if (ip.ends_with(".local")) {
                ip = ResolveMDNS(ip);
            }
        } else {
            ip = "0.0.0.0";
        }

        if (json.contains("hostport")) {
            port = json["hostport"];
        } else {
            port = 8080;
        }

        if (json.contains("servers") && json["servers"].is_array()) {
            for (const auto &servJson : json["servers"]) {
                DashsrvConfigServer serverConfig;
                serverConfig.type = servJson["type"];
                if (serverConfig.type == "minecraft") {
                    if (!servJson.contains("ip") || !servJson.contains("port") || !servJson.contains("version")) {
                        throw std::runtime_error("malformed config.json (in minecraft server type)");
                    }

                    std::string extraDomain = "";
                    if (servJson.contains("extra-domain")) {
                        extraDomain = servJson["extra-domain"];
                    }

                    std::string sip = servJson["ip"];
                    if (sip.ends_with(".local")) {
                        sip = ResolveMDNS(sip);
                    }

                    serverConfig.server = (DashsrvConfigServer::Minecraft){
                        .ip = sip, .port = servJson["port"], .version = servJson["version"], .extraDomain = extraDomain
                    };
                } else if (serverConfig.type == "jellyfin") {
                    if (!servJson.contains("ip") || !servJson.contains("port")) {
                        throw std::runtime_error("malformed config.json (in jellyfin server type)");
                    }

                    std::string sip = servJson["ip"];
                    if (sip.ends_with(".local")) {
                        sip = ResolveMDNS(sip);
                    }

                    serverConfig.server = (DashsrvConfigServer::Jellyfin){ .ip = sip, .port = servJson["port"] };
                } else if (serverConfig.type == "dashboard") {
                    if (!servJson.contains("ip") || !servJson.contains("port")) {
                        throw std::runtime_error("malformed config.json (in jellyfin server type)");
                    }

                    std::string sip = servJson["ip"];
                    if (sip.ends_with(".local")) {
                        sip = ResolveMDNS(sip);
                    }

                    serverConfig.server = (DashsrvConfigServer::Dashboard){ .ip = sip, .port = servJson["port"] };
                } else {
                    throw std::runtime_error("malformed config.json (unknown server type '" + serverConfig.type + "')");
                }

                servers.push_back(serverConfig);
            }
        }
    }
}
