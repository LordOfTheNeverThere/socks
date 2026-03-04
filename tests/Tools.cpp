#include "gtest/gtest.h"
#include "../include/socks/Tools.h"

#include <netinet/in.h>

TEST(MethodTesting, add16BitOnesComplement) {

    uint16_t a = htons(0xFFFF);
    uint16_t b = htons(0x000F);

    uint8_t packetPtr[sizeof(a) + sizeof(b)] = {};
    memcpy(packetPtr, &a, sizeof(a));
    memcpy(packetPtr + sizeof(a), &b, sizeof(b));


    EXPECT_EQ(Tools::add16BitOnesComplement(packetPtr, sizeof(a) + sizeof(b)), 0x000F);
}


TEST(MethodTesting, ICMPChecksumValidation) {
    uint8_t packet[] = { 0x08, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x01 };
    uint16_t checksum = htons(~Tools::add16BitOnesComplement(packet, 8));
    memcpy(packet + 2, &checksum, 2);
    uint16_t validation = Tools::add16BitOnesComplement(packet, 8);

    EXPECT_EQ(validation, 0xFFFF);
}

