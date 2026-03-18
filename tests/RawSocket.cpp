#include "socks/RawSocket.h"
#include <netinet/ip_icmp.h>
#include "gtest/gtest.h"
#include "socks/LocalHost.h"


TEST(Misc, checkingICMPChecksumCalculation) {
    const size_t headerSize = sizeof(icmphdr);
    const size_t nanosecsSize = sizeof(uint64_t);

    // create memory buffer to load the packet unto
    uint8_t packet[headerSize + nanosecsSize] = {};
    uint16_t expectedID {69};
    RawSocket::constructICMPPacket(packet, headerSize, nanosecsSize, 0, expectedID);

    EXPECT_EQ(Tools::add16BitOnesComplement(packet, headerSize + nanosecsSize), std::numeric_limits<uint16_t>::max());

    uint16_t id {};
    std::memcpy(&id, packet + sizeof(uint8_t)*2 + sizeof(uint16_t), sizeof(uint16_t));
    EXPECT_EQ(ntohs(id), expectedID); // Check that ID was created successfully
}


TEST(MethodChecking, sendPingReceivePing) {

    LocalHost myMachine {true};
    std::string destinationIP {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(destinationIP, AF_INET);
    std::string senderIP {interfaceWithGateway.getIPAddress()};

    RawSocket socket {AF_INET, IPPROTO_ICMP};
    socket.sendPing(destinationIP);

    uint8_t packet[IP_MAXPACKET] {};
    socket.receivePing(packet, destinationIP, 0 , 0);

    uint64_t nanosecs {};
    memcpy( &nanosecs,packet + sizeof(icmp), sizeof(uint64_t));
    nanosecs = be64toh(nanosecs);
    uint64_t currNanosecs = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
    EXPECT_TRUE(currNanosecs - nanosecs < 5000000000); // There was at most a 5 second gap between the time the request was sent and the reply fetched
    std::cout << currNanosecs << '\n' << nanosecs << '\n';

    uint8_t ipHeaderReceiveBuffer[sizeof(ip)] {};
    memcpy(&ipHeaderReceiveBuffer,packet, sizeof(ip));
    // The IP header is the first element of the packet +  then ICMP header  + Payload. If this was an ICMP error it would be
    // [IP Header] + [ICMP Header] + [Your Original IP Header] + [Your Original ICMP Header] + [Payload]
    IPv4Header ipHeaderReceive {IPv4Header(ipHeaderReceiveBuffer)};

    EXPECT_EQ(senderIP, ipHeaderReceive.getDestStr());
    EXPECT_EQ(destinationIP, ipHeaderReceive.getSourceStr());
}



TEST(MethodChecking, sendPingReceivePingWithIPHeader) {

    LocalHost myMachine {true};
    std::string destinationIP {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(destinationIP, AF_INET);
    std::string senderIP {interfaceWithGateway.getIPAddress()};

    RawSocket socket {AF_INET, IPPROTO_ICMP};
    uint8_t ipHeaderSendBuffer[sizeof(ip)] {};
    IPv4Header ipHeaderSend {IPv4Header(ipHeaderSendBuffer, senderIP.c_str(), destinationIP.c_str(),0, 0)}; // these values will be filled when sending the ping
    socket.setIPHeader(ipHeaderSend);
    socket.sendPing(destinationIP, 1, 2);

    uint8_t packet[IP_MAXPACKET] {};
    socket.receivePing(packet, destinationIP, 1 , 2);

    uint64_t nanosecs {};
    std::memcpy( &nanosecs,packet + sizeof(icmp), sizeof(uint64_t));
    nanosecs = be64toh(nanosecs);
    uint64_t currNanosecs = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
    EXPECT_TRUE(currNanosecs - nanosecs < 5000000000); // There was at most a 5 second gap between the time the request was sent and the reply fetched
    std::cout << currNanosecs << '\n' << nanosecs << '\n';


    uint8_t ipHeaderReceiveBuffer[sizeof(ip)] {};
    std::memcpy(&ipHeaderReceiveBuffer,packet, sizeof(ip));
    // The IP header is the first element of the packet +  then ICMP header  + Payload. If this was an ICMP error it would be
    // [IP Header] + [ICMP Header] + [Your Original IP Header] + [Your Original ICMP Header] + [Payload]
    IPv4Header ipHeaderReceive {IPv4Header(ipHeaderReceiveBuffer)};

    EXPECT_EQ(senderIP, ipHeaderReceive.getDestStr());
    EXPECT_EQ(destinationIP, ipHeaderReceive.getSourceStr());
}

TEST(MethodChecking, sendReceiveARPPing) {

    LocalHost myMachine {true};
    std::string destinationIP {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(destinationIP, AF_INET);

    std::string senderIP {interfaceWithGateway.getIPAddress()};
    std::string senderMAC {interfaceWithGateway.getMacAddress()};
    std::string interfaceName {interfaceWithGateway.getInterfaceName()};


    RawSocket socket {AF_PACKET, htons(ETH_P_ARP)};
    socket.sendArpEchoRequest(destinationIP, senderMAC, senderIP, interfaceName);

    uint8_t recvBuffer[ETH_FRAME_LEN] {};
    socket.receiveArpEchoReply(recvBuffer);

    IPv4ToMACARPPacket arpResponse {};
    std::memcpy(&arpResponse, recvBuffer + sizeof(ether_header), sizeof(IPv4ToMACARPPacket));
    char srcIPAddressThatWasReceived[INET_ADDRSTRLEN] {};
    inet_ntop(AF_INET, &arpResponse.srcIPv4, srcIPAddressThatWasReceived,INET_ADDRSTRLEN);

    EXPECT_TRUE(strcmp(srcIPAddressThatWasReceived, destinationIP.c_str()) == 0);
}



TEST(MethodChecking, sendReceiveARPPingWithIPQuery) {

    LocalHost myMachine {true};
    std::string destinationIP {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(destinationIP, AF_INET);

    std::string senderIP {interfaceWithGateway.getIPAddress()};
    std::string senderMAC {interfaceWithGateway.getMacAddress()};
    std::string interfaceName {interfaceWithGateway.getInterfaceName()};


    RawSocket socket {AF_PACKET, htons(ETH_P_ARP)};
    socket.sendArpEchoRequest(destinationIP, senderMAC, senderIP, interfaceName);

    uint8_t recvBuffer[ETH_FRAME_LEN] {};
    socket.receiveArpEchoReply(recvBuffer, destinationIP);

    IPv4ToMACARPPacket arpResponse {};
    std::memcpy(&arpResponse, recvBuffer + sizeof(ether_header), sizeof(IPv4ToMACARPPacket));
    char srcIPAddressThatWasReceived[INET_ADDRSTRLEN] {};
    inet_ntop(AF_INET, &arpResponse.srcIPv4, srcIPAddressThatWasReceived,INET_ADDRSTRLEN);

    EXPECT_TRUE(strcmp(srcIPAddressThatWasReceived, destinationIP.c_str()) == 0);
}