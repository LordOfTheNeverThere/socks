#include "socks/RawSocket.h"
#include <netinet/ip_icmp.h>
#include "gtest/gtest.h"


TEST(Misc, checkingICMPChecksumCalculation) {
    const size_t headerSize = sizeof(icmphdr);
    const size_t nanosecsSize = sizeof(uint64_t);

    // create memory buffer to load the packet unto
    uint8_t packet[headerSize + nanosecsSize] = {};
    RawSocket::constructICMPPacket(packet, headerSize, nanosecsSize, 0);

    EXPECT_EQ(Tools::add16BitOnesComplement(packet, headerSize + nanosecsSize), std::numeric_limits<uint16_t>::max());

    uint16_t id {};
    memcpy(&id, packet + sizeof(uint8_t)*2 + sizeof(uint16_t), sizeof(uint16_t));
    EXPECT_EQ(ntohs(id), getpid()); // Check that ID was created successfully
}


TEST(MethodChecking, sendPingReceivePing) {

    std::string senderIP {"10.7.7.28"};
    std::string destinationIP {"8.8.8.8"};

    RawSocket socket {AF_INET, IPPROTO_ICMP};
    socket.sendPing(destinationIP);

    uint8_t packet[IP_MAXPACKET] {};
    socket.receivePing(packet);

    uint64_t nanosecs {};
    memcpy( &nanosecs,packet + sizeof(icmp), sizeof(uint64_t));
    nanosecs = be64toh(nanosecs);
    uint64_t currNanosecs = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
    EXPECT_TRUE(currNanosecs - nanosecs < 5000000000); // There was at most a 5 second gap between the time the request was sent and the reply fetched
    std::cout << currNanosecs << '\n' << nanosecs << '\n';

    ip ipHeader {};
    memcpy( &ipHeader,packet, sizeof(ip)); // The IP header is the first element of the packet +  then ICMP header  + Payload. If this was an ICMP error it would be
    // [IP Header] + [ICMP Header] + [Your Original IP Header] + [Your Original ICMP Header] + [Payload]

    char initialSenderIP[INET_ADDRSTRLEN];
    char initialDestinationIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(ipHeader.ip_src), initialDestinationIP, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &(ipHeader.ip_dst), initialSenderIP, INET_ADDRSTRLEN); // roles have reversed whence of the reply
    EXPECT_TRUE(strcmp(initialDestinationIP, destinationIP.c_str())==0);
    EXPECT_TRUE(strcmp(initialSenderIP, senderIP.c_str())==0);
}