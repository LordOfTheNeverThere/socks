//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_TOOLS_H
#define SOCKS_TOOLS_H
#include <iomanip>
#include <sstream>
#include <netpacket/packet.h>

#include "types.h"

class Tools {
    public:

    Tools() {};

    uint16_t static add16BitOnesComplement(const uint8_t* packetPtr, const uint64_t& packetSize) {
        uint32_t bucketSum = 0;
        for (int i = 0; i < packetSize; i = i + 2) {
            uint16_t twoBytes {0};
            if (i == packetSize -1) { //handling oddbytes
                twoBytes = (packetPtr[i] << 8);
            } else {
                twoBytes = (packetPtr[i] << 8) + packetPtr[i + 1];
            }
            bucketSum = bucketSum + twoBytes;
        }

        while (bucketSum > 0xFFFF) {
            const uint16_t carryOver = (bucketSum >> 16);
            bucketSum = bucketSum & 0xFFFF;
            bucketSum = bucketSum + carryOver;
        }

        return static_cast<uint16_t>(bucketSum);
    }


    static std::string macToString(struct sockaddr_ll *s) {
        std::ostringstream oss;

        for (int i = 0; i < s->sll_halen; ++i) {
            // Make the stream interpret the following values as hexadecimal, each input will be at least two digits wide, and if there is a zero it should be printed as such and not as space
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(s->sll_addr[i]);
        }

        return oss.str();
    }
};

#endif //SOCKS_TOOLS_H