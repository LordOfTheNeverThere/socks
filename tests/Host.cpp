

#include "gtest/gtest.h"
#include "socks/LocalHost.h"
#include "socks/RawSocket.h"


TEST(MethodChecking, PopulateObject) {
    LocalHost myMachine {true};
    EXPECT_NE("", myMachine.getName());
    EXPECT_NE("", myMachine.getIPAddress());
    EXPECT_NE("", myMachine.getNetworkMask());
    EXPECT_NE("", myMachine.getMacAddress());
}

TEST(MethodChecking, doNotPopulateObject) {
    LocalHost myMachine {false};
    EXPECT_EQ("", myMachine.getName());
    EXPECT_EQ("", myMachine.getIPAddress());
    EXPECT_EQ("", myMachine.getNetworkMask());
    EXPECT_EQ("", myMachine.getMacAddress());
}

TEST(MethodChecking, selectNonExistentName) {
    LocalHost myMachine {false};
    EXPECT_THROW(try {
        myMachine.getDataFromCurrentHost(2, "This Interface Name will never exit!");
    } catch (GenericException ge) {
        EXPECT_STREQ(ge.what(), "It was not possible to populate the data");
        throw;
    }, GenericException);
}



TEST(MethodChecking, PopulateHostFromARP) {

    LocalHost myMachine {true};
    std::string senderIP {myMachine.getIPAddress()};
    std::string senderMAC {myMachine.getMacAddress()};
    std::string interfaceName {myMachine.getInterfaceName()};
    std::string destinationIP {Tools::getDefaultGateway()};

    RawSocket socket {AF_PACKET, htons(ETH_P_ARP)};
    socket.sendArpEchoRequest(destinationIP, senderMAC, senderIP, interfaceName);

    uint8_t recvBuffer[ETH_FRAME_LEN] {};
    socket.receiveArpEchoReply(recvBuffer);

    Host externalHost {};
    externalHost.populateFromARPEchoReply(recvBuffer);
    std::cout << externalHost << '\n';

    EXPECT_EQ(externalHost.getIPAddress(), destinationIP);
    EXPECT_NE(externalHost.getName(), "");
    EXPECT_NE(externalHost.getMacAddress(), "");
}