#include "socks/RawSocket.h"
#include <netinet/ip_icmp.h>
#include "gtest/gtest.h"


TEST(Misc, checkingICMPChecksumCalculation) {
    const size_t headerSize = sizeof(icmp);
    const size_t nanosecsSize = sizeof(uint64_t);

    // create memory buffer to load the packet unto
    uint8_t packet[headerSize + nanosecsSize] = {};
    RawSocket::constructICMPPacket(packet, headerSize, nanosecsSize);

    EXPECT_EQ(Tools::add16BitOnesComplement(packet, headerSize + nanosecsSize), std::numeric_limits<uint16_t>::max());
}


TEST(MethodChecking, sendPingReceivePing) {

    RawSocket socket {AF_INET, IPPROTO_ICMP};

    socket.sendPing("10.7.7.1");
}
