#include "gtest/gtest.h"
#include "../include/socks/Tools.h"

#include <netinet/in.h>

TEST(MethodChecking, add16BitOnesComplement) {

    uint16_t a = htons(0xFFFF);
    uint16_t b = htons(0x000F);

    uint8_t packetPtr[sizeof(a) + sizeof(b)] = {};
    memcpy(packetPtr, &a, sizeof(a));
    memcpy(packetPtr + sizeof(a), &b, sizeof(b));


    EXPECT_EQ(Tools::add16BitOnesComplement(packetPtr, sizeof(a) + sizeof(b)), 0x000F);
}


TEST(MethodChecking, ICMPChecksumValidation) {
    uint8_t packet[] = { 0x08, 0x00, 0x00, 0x00, 0x12, 0x34, 0x00, 0x01 };
    uint16_t checksum = htons(~Tools::add16BitOnesComplement(packet, 8));
    memcpy(packet + 2, &checksum, 2);
    uint16_t validation = Tools::add16BitOnesComplement(packet, 8);

    EXPECT_EQ(validation, 0xFFFF);
}


TEST(MethodChecking, convertStringToMac) {
    std::array<uint8_t,6> result = Tools::stringToMac("F0E1D2C3B4A5");
    std::array<uint8_t,6> expected { 15*16 + 0, 14*16 + 1, 13*16 + 2, 12*16 + 3, 11*16 + 4, 10*16 + 5 };
    for (int i = 0; i < result.size(); ++i) {
        EXPECT_EQ(result.at(i), expected.at(i));
    }
}

TEST(MethodChecking, convertStringToMacWrongFormat) {
    EXPECT_THROW({
        try {
            Tools::stringToMac("F0:E1:D2:C3:B4:A5");
        } catch( const GenericException& e ) {
            EXPECT_STREQ("The stringToMac utility function only allows strings with 12 characters. So no other delimiter characters are allowed"
                , e.what());
            throw; // Re-throw so EXPECT_THROW sees it
        }
    }, GenericException);
}

TEST(MethodChecking, convertStringToMacWrongHex) {
    EXPECT_THROW({
    try {
        Tools::stringToMac("F0E1D2C3Z4A5");
    } catch( const GenericException& e ) {
        EXPECT_STREQ("Unknown Hexadecimal digit Z"
            , e.what());
        throw; // Re-throw so EXPECT_THROW sees it
    }
}, GenericException);
}

