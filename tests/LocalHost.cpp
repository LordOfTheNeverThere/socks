

#include "gtest/gtest.h"
#include "socks/LocalHost.h"
#include "socks/RawSocket.h"


TEST(MethodChecking, PopulateObject) {
    LocalHost myMachine {true};

    EXPECT_TRUE(!myMachine.getInterfaces().empty());
}

TEST(MethodChecking, findAnInterfaceBelongingToANetworkIPv4) {

    LocalHost myMachine {true};
    std::string destinationIP {Tools::getDefaultGateway()};
    InternalInterface interfaceWithGateway = myMachine.getInterfaceFromSubnet(destinationIP, AF_INET);

    EXPECT_EQ(interfaceWithGateway.getIPVersion(), AF_INET);
    EXPECT_NE(interfaceWithGateway.getInterfaceName(), "");
    EXPECT_NE(interfaceWithGateway.getIPAddress(), "");
    EXPECT_NE(interfaceWithGateway.getNetworkMask(), "");
    EXPECT_NE(interfaceWithGateway.getMacAddress(), "");
    EXPECT_NE(interfaceWithGateway.getMacVendor(), "Unknown Vendor");
}

TEST(MethodChecking, findAnInterfaceBelongingToANetworkIPv6) {

    LocalHost myMachine {true};
    InternalInterface ipv6Interface {};
    for (auto interface: myMachine.getInterfaces()) {
        if (interface.getIPVersion() == AF_INET6) {
            ipv6Interface = interface;
        }
    }
    EXPECT_NE(ipv6Interface.getIPAddress(), "");

    std::string queryString {ipv6Interface.getIPAddress().substr(0, ipv6Interface.getIPAddress().size() - 1) + '0'};
    InternalInterface ipv6InterfaceViaSubnetMethod = myMachine.getInterfaceFromSubnet(queryString, AF_INET6);

    EXPECT_EQ(ipv6Interface, ipv6InterfaceViaSubnetMethod);
}


TEST(MethodChecking, PopulateHostFromARP) {

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

    ExternalInterface externalHost {};
    externalHost.populateFromARPEchoReply(recvBuffer);
    std::cout << externalHost << '\n';

    EXPECT_EQ(externalHost.getIPAddress(), destinationIP);
    EXPECT_NE(externalHost.getMacVendor(), "");
    EXPECT_NE(externalHost.getMacAddress(), "");
}