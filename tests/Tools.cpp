#include "gtest/gtest.h"
#include "../include/socks/Tools.h"

TEST(MethodTesting, add16BitOnesComplement) {

    uint16_t a = 0xFFFF;
    uint16_t b = 0x000F;

    EXPECT_EQ(Tools::add16BitOnesComplement(a,b), b);
}


TEST(MethodTesting, add16BitOnesComplementDoubleWrapArroundCarryOver) {

    uint16_t a = 0xFFFF;
    uint16_t b = 0x000F;

    EXPECT_EQ(Tools::add16BitOnesComplement(a,b), b);
}
