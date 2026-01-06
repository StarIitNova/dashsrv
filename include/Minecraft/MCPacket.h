#ifndef DASHSRV_MCPACKET_H__
#define DASHSRV_MCPACKET_H__

#include <vector>
#include <cstdint>

// Handshake information
#define MC_PACKET_HANDSHAKE 0x00
#define MC_PACKET_HANDSHAKE_LEGACY 0xFE

// Serverbound packet stages
#define MC_HANDSHAKE_INTENT_STATUS 0x01
#define MC_HANDSHAKE_INTENT_LOGIN 0x02
#define MC_HANDSHAKE_INTENT_TRANSFER 0x03

// Clientbound status packet ids
#define MC_PACKETACC_STATUS 0x00 // status_response
#define MC_PACKETACC_PONG 0x01 // pong_response

// Serverbound status packet ids
#define MC_PACKETID_STATUS 0x00 // status_request
#define MC_PACKETID_PING 0x01 // ping_request

// Clientbound login packet ids
#define MC_PACKETACC_DISCONECT 0x00 // login_disconnect
#define MC_PACKETACC_ENCRYPTION_REQ 0x01 // hello
#define MC_PACKETACC_LOGIN_SUCCESS 0x02 // login_finished
#define MC_PACKETACC_SET_COMPRESS 0x03 // login_compression
#define MC_PACKETACC_LOGIN_PLUGIN_REQ 0x04 // custom_query
#define MC_PACKETACC_COOKIE_REQ 0x05 // cookie_request

// Serverbound login packet ids
#define MC_PACKETID_LOGIN_START 0x00 // hello
#define MC_PACKETID_ENCRYPTION_RESP 0x01 // key
#define MC_PACKETID_LOGIN_PLUGIN_RES 0x02 // custom_query_answer
#define MC_PACKETID_LOGIN_ACK 0x03 // login_acknowledged
#define MC_PACKETID_COOKIE_RESP 0x04 // cookie_response

// Clientbound configuration packet ids
#define MC_PACKETACC_COOKIE_REQ_CFG 0x00 // cookie_request
#define MC_PACKETACC_PLUGIN_MSG 0x01 // custom_payload
#define MC_PACKETACC_DISCONNECT_CFG 0x02 // disconnect
#define MC_PACKETACC_FINISH_CFG 0x03 // finish_configuration
#define MC_PACKETACC_KEEPALIVE 0x04 // keep_alive
#define MC_PACKETACC_PING_CFG 0x05 // ping
#define MC_PACKETACC_RESET_CHAT 0x06 // reset_chat
#define MC_PACKETACC_REGISTRY_DATA 0x07 // registry_data
#define MC_PACKETACC_REMOVE_RES_PACK 0x08 // resource_pack_pop
#define MC_PACKETACC_ADD_RES_PACK 0x09 // resource_pack_push
#define MC_PACKETACC_STORE_COOKIE 0x0A // store_cookie
#define MC_PACKETACC_TRANSFER 0x0B // transfer
#define MC_PACKETACC_FEATURES 0x0C // update_enabled_features
#define MC_PACKETACC_TAGS 0x0D // update_tags
#define MC_PACKETACC_KNOWN_PACKS 0x0E // select_known_packs
#define MC_PACKETACC_CUSTOM_REPORT_DETAILS 0x0F // custom_report_details
#define MC_PACKETACC_SERVER_LINKS 0x10 // server_links
#define MC_PACKETACC_CLEAR_DIALOG 0x11 // clear_dialog
#define MC_PACKETACC_SHOW_DIALOG 0x12 // show_dialog
#define MC_PACKETACC_CODE_OF_CONDUCT 0x13 // code_of_conduct

// Serverbound configuration packet ids
#define MC_PACKETID_CLIENT_INFO 0x00 // client_information
#define MC_PACKETID_COOKIE_RESP_CFG 0x01 // cookie_response
#define MC_PACKETID_PLUGIN_MSG 0x02 // custom_payload
#define MC_PACKETID_ACK_FINISH_CFG 0x03 // finish_configuration
#define MC_PACKETID_KEEPALIVE 0x04 // keep_alive
#define MC_PACKETID_PONG_CFG 0x05 // pong
#define MC_PACKETID_RES_PACK_RESP 0x06 // resource_pack
#define MC_PACKETID_KNOWN_PACKS 0x07 // select_known_packs
#define MC_PACKETID_CLICK_ACT 0x08 // custom_click_action
#define MC_PACKETID_ACCEPT_CODE_OF_CONDUCT 0x09 // accept_code_of_conduct

// I'm not providing play packets. If you want to hack minecraft or remake the server from the ground up do that in your own time.
// List of packets taken from https://minecraft.wiki/w/Java_Edition_protocol/Packets

namespace Minecraft {
    namespace packetool {
        void WriteVarInt(std::vector<uint8_t> &data, std::vector<uint8_t>::iterator it, int32_t val);
        void WriteVarInt(std::vector<uint8_t> &data, int32_t val);
        void WriteVarLong(std::vector<uint8_t> &data, std::vector<uint8_t>::iterator it, int64_t val);
        void WriteVarLong(std::vector<uint8_t> &data, int64_t val);

        int32_t ReadVarInt(uint8_t *&data);
        int64_t ReadVarLong(uint8_t *&data);

        void WriteShort(std::vector<uint8_t> &data, int16_t val);
        void WriteInt(std::vector<uint8_t> &data, int32_t val);
        void WriteLong(std::vector<uint8_t> &data, int64_t val);

        int16_t ReadShort(const uint8_t *data);
        int32_t ReadInt(const uint8_t *data);
        int64_t ReadLong(const uint8_t *data);

        uint64_t GetTimeMS();
    }
}

#endif // DASHSRV_MCPACKET_H__
