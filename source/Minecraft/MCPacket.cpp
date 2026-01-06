#include <Minecraft/MCPacket.h>

#include <chrono>

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

namespace Minecraft {
    namespace packetool {
        void WriteVarInt(std::vector<uint8_t> &data, std::vector<uint8_t>::iterator it, int32_t val) {
            std::vector<uint8_t> inserting;
            uint32_t cast = (uint32_t)val;
            while (true) {
                if ((cast & ~SEGMENT_BITS) == 0) {
                    inserting.push_back(cast);
                    break;
                }

                inserting.push_back((cast & SEGMENT_BITS) | CONTINUE_BIT);
                cast >>= 7;
            }

            data.insert(it, inserting.begin(), inserting.end());
        }
        
        void WriteVarInt(std::vector<uint8_t> &data, int32_t val) {
            WriteVarInt(data, data.begin() + data.size(), val);
        }
        
        void WriteVarLong(std::vector<uint8_t> &data, std::vector<uint8_t>::iterator it, int64_t val) {
            std::vector<uint8_t> inserting;
            uint64_t cast = (uint64_t)val;
            
            while (true) {
                if ((cast & ~((uint64_t) SEGMENT_BITS)) == 0) {
                    inserting.push_back(cast);
                    break;
                }

                inserting.push_back((cast & SEGMENT_BITS) | CONTINUE_BIT);
                cast >>= 7;
            }

            data.insert(it, inserting.begin(), inserting.end());
        }
        
        void WriteVarLong(std::vector<uint8_t> &data, int64_t val) {
            WriteVarLong(data, data.begin() + data.size(), val);
        }

        int32_t ReadVarInt(uint8_t *&data) {
            uint32_t value = 0;
            uint32_t position = 0;
            uint8_t currentByte;

            while (true) {
                currentByte = *data++;
                value |= (currentByte & SEGMENT_BITS) << position;

                if ((currentByte & CONTINUE_BIT) == 0) break;

                position += 7;
                if (position >= 32) break;
            }

            return (int32_t)value;
        }
        
        int64_t ReadVarLong(uint8_t *&data) {
            uint64_t value = 0;
            uint32_t position = 0;
            uint8_t currentByte;

            while (true) {
                currentByte = *data++;
                value |= (long) (currentByte & SEGMENT_BITS) << position;

                if ((currentByte & CONTINUE_BIT) == 0) break;

                position += 7;

                if (position >= 64) break;
            }

            return (int64_t)value;
        }

        void WriteShort(std::vector<uint8_t> &data, int16_t val) {
            uint16_t cast = (uint16_t)val;
            data.push_back((uint8_t)((cast >> 8) & 0xFF));
            data.push_back((uint8_t)(cast & 0xFF));
        }
        
        void WriteInt(std::vector<uint8_t> &data, int32_t val) {
            uint32_t cast = (uint32_t)val;
            data.push_back((uint8_t)((cast >> 24) & 0xFF));
            data.push_back((uint8_t)((cast >> 16) & 0xFF));
            data.push_back((uint8_t)((cast >> 8) & 0xFF));
            data.push_back((uint8_t)(cast & 0xFF));
        }
        
        void WriteLong(std::vector<uint8_t> &data, int64_t val) {
            uint64_t cast = (uint64_t)val;
            data.push_back((uint8_t)((cast >> 56) & 0xFF));
            data.push_back((uint8_t)((cast >> 48) & 0xFF));
            data.push_back((uint8_t)((cast >> 40) & 0xFF));
            data.push_back((uint8_t)((cast >> 32) & 0xFF));
            data.push_back((uint8_t)((cast >> 24) & 0xFF));
            data.push_back((uint8_t)((cast >> 16) & 0xFF));
            data.push_back((uint8_t)((cast >> 8) & 0xFF));
            data.push_back((uint8_t)(cast & 0xFF));
        }
        
        int16_t ReadShort(const uint8_t* data) {
            return (int16_t)(
                             (uint16_t(data[0]) << 8) |
                             (uint16_t(data[1]))
                             );
        }

        int32_t ReadInt(const uint8_t* data) {
            return (int32_t)(
                             (uint32_t(data[0]) << 24) |
                             (uint32_t(data[1]) << 16) |
                             (uint32_t(data[2]) << 8)  |
                             (uint32_t(data[3]))
                             );
        }

        int64_t ReadLong(const uint8_t* data) {
            return (int64_t)(
                             (uint64_t(data[0]) << 56) |
                             (uint64_t(data[1]) << 48) |
                             (uint64_t(data[2]) << 40) |
                             (uint64_t(data[3]) << 32) |
                             (uint64_t(data[4]) << 24) |
                             (uint64_t(data[5]) << 16) |
                             (uint64_t(data[6]) << 8)  |
                             (uint64_t(data[7]))
                             );
        }

        uint64_t GetTimeMS() {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock().now().time_since_epoch()).count();
        }
    }
}
