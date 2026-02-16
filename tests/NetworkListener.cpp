#include "socks/NetworkListener.h"

#include <gtest/gtest.h>


TEST(MethodChecking, LocalhostResponse) {
    NetworkListener socket;
    socket.fetchInterfacesToBind("8080", 0, SOCK_RAW);

    for (const auto& interface: socket.getInterfaces()) {
        EXPECT_EQ(interface.port, 8080);
        EXPECT_TRUE(interface.family == AF_INET || interface.family == AF_INET6 );
        EXPECT_TRUE(interface.socketType == SOCK_DGRAM || interface.socketType == SOCK_STREAM || interface.socketType == SOCK_RAW );
        EXPECT_TRUE(interface.ipString == "::" || interface.ipString == "0.0.0.0");
    }
}


TEST(MethodChecking, RemoteHostResponse) {

    NetworkListener socket;
    socket.fetchInterfacesToConnect("localhost","65000", AF_INET6, SOCK_RAW);

    for (const auto& interface: socket.getInterfaces()) {
        EXPECT_EQ(interface.port, 65000);
        EXPECT_TRUE(interface.family == AF_INET6);
        EXPECT_TRUE(interface.socketType == SOCK_DGRAM || interface.socketType == SOCK_STREAM || interface.socketType == SOCK_RAW );
        EXPECT_TRUE(interface.ipString == "::1"); // localhost ipv6
    }
}
