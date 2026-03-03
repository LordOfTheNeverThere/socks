//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_TOOLS_H
#define SOCKS_TOOLS_H
#include "types.h"

class Tools {
    public:

    Tools() {};

    uint16_t static add16BitOnesComplement(uint16_t a, uint16_t b) {
        uint32_t bucketSum = 0;
        bucketSum += a;
        bucketSum += b;

        while (bucketSum > 0xFFFF) {
            uint32_t carryOver = (bucketSum & ~0xFFFF) >> 16;
            bucketSum = bucketSum & 0xFFFF;
            bucketSum = bucketSum + carryOver;
        }

        return static_cast<uint16_t>(bucketSum);
    }

    uint16_t static icmpChecksum(uint64_t& header) {
        uint16_t checkSum = 0;
        for (uint64_t mask = 0xFFFF; mask <= (mask << (sizeof(header)*8 - 16)); mask <<= 16) {

            checkSum = add16BitOnesComplement(checkSum,(header & mask));
        }
        return checkSum;
    }
};

#endif //SOCKS_TOOLS_H