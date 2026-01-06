#include <Minecraft/Status.h>

#include <Minecraft/MCPacket.h>
#include <Minecraft/MCQuery.h>

#include <nlohmann/json.hpp>

#include <vector>
#include <chrono>
#include <cstdint>
#include <cassert>

namespace Minecraft {
    using namespace packetool;
    
    namespace inte__ {
        std::vector<uint8_t> BuildHandshakePacket(const MCServer &server);
        std::vector<uint8_t> BuildStatusRequestPacket();
        std::vector<uint8_t> BuildPingRequestPacket();
    }

    MCStatus QueryServer(const MCServer &server) {
        MCStatus status;
        
        std::vector<uint8_t> handshakePacket = inte__::BuildHandshakePacket(server);
        std::vector<uint8_t> statusPacket = inte__::BuildStatusRequestPacket();
        std::vector<uint8_t> pingPacket = inte__::BuildPingRequestPacket();

        MCQueryState query;
        QueryMinecraft(query, server, [&](MCSendBytes sendBytes) {
            sendBytes(handshakePacket);
            sendBytes(statusPacket);
            sendBytes(pingPacket);
        });

        status.Online = false;
        status.Error = "No error";
        
        if (!query.Success) {
            status.Error = "Server failed to respond or denied the request";
            return status;
        }

        uint8_t *statusStart = query.Recv.data();
        uint8_t *pingStart;
        
        // parsing
        {
            int32_t statusLen = ReadVarInt(statusStart);
            pingStart = statusStart + statusLen;
            uint8_t *statusEnd = pingStart;
            int32_t statusPacketId = ReadVarInt(statusStart);
            __attribute__((unused)) int32_t jsonStringLen = ReadVarInt(statusStart);
            __attribute__((unused)) int32_t pingLen = ReadVarInt(pingStart);
            int32_t pingPacketId = ReadVarInt(pingStart);

            if (pingPacketId != MC_PACKETACC_PONG) {
                status.Error = "Server responded to ping with a packet other than pong";
                return status;
            }

            if (statusPacketId != MC_PACKETACC_STATUS) {
                status.Error = "Server responded to status with a packet other than status";
                return status;
            }
            
            uint64_t pingResponse = ReadLong(pingStart);
            status.PingMS = packetool::GetTimeMS() - pingResponse;

            // now parse json
            std::string_view sv(reinterpret_cast<const char *>(statusStart), statusEnd - statusStart);
            auto doc = nlohmann::json::parse(sv, nullptr, false);

            if (!doc.is_discarded()) {
                if (doc.contains("description")) {
                    status.MOTD = doc["description"];
                }

                if (doc.contains("players")) {
                    status.Players.Online = doc["players"]["online"];
                    status.Players.Max = doc["players"]["max"];
                }

                if (doc.contains("version")) {
                    status.Version.Name = doc["version"]["name"];
                    status.Version.Protocol = doc["version"]["protocol"];
                }

                if (doc.contains("favicon")) {
                    status.Icon = doc["favicon"];
                }

                status.Online = true;
            } else {
                status.Error = "JSON parser discarded packet (corrupt JSON)";
                return status;
            }
        }

        return status;
    }

    namespace inte__ {
        std::vector<uint8_t> BuildHandshakePacket(const MCServer &server) {
            std::vector<uint8_t> packet;

            {
                packet.push_back(MC_PACKET_HANDSHAKE);
                WriteVarInt(packet, server.ProtocolVersion);
                WriteVarInt(packet, server.IP.length());
                packet.insert(packet.begin() + packet.size(), server.IP.begin(), server.IP.end());
                WriteShort(packet, server.Port);
                WriteVarInt(packet, MC_HANDSHAKE_INTENT_STATUS);
                WriteVarInt(packet, packet.begin(), packet.size());
            }
            
            return packet;
        }
        
        std::vector<uint8_t> BuildStatusRequestPacket() {
            std::vector<uint8_t> packet;

            {
                packet.push_back(MC_PACKETID_STATUS);
                WriteVarInt(packet, packet.begin(), packet.size());
            }
            
            return packet;
        }
        
        std::vector<uint8_t> BuildPingRequestPacket() {
            std::vector<uint8_t> packet;

            {
                uint64_t timestampMS = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                packet.push_back(MC_PACKETID_PING);
                WriteLong(packet, timestampMS);
                WriteVarInt(packet, packet.begin(), packet.size());
            }
            
            return packet;
        }
    }
}
