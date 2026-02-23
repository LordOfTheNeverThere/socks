#include "socks/NetworkListener.h"

#include <gtest/gtest.h>


TEST(MethodChecking, LocalhostResponse) {
    NetworkListener socket;
    socket.fetchInterfacesToBind("8080", 0, 0);

    for (const auto& interface: socket.getInterfaces()) {
        EXPECT_EQ(interface.port, 8080);
        EXPECT_TRUE(interface.family == AF_INET || interface.family == AF_INET6 );
        EXPECT_TRUE(interface.socketType == SOCK_DGRAM || interface.socketType == SOCK_STREAM || interface.socketType == SOCK_RAW );
        EXPECT_TRUE(interface.ipString == "::" || interface.ipString == "0.0.0.0");
    }
}


TEST(MethodChecking, LocalhostResponseError) {
    NetworkListener socket;
    try {
        socket.fetchInterfacesToBind("8080", 7, 0);
    } catch (std::string error) {
        EXPECT_EQ(error, "The name resolution was not able to find any viable interfaces for the socket connection to localhost on port: 8080 and IP version: 7.\nPrompting the error with status code: ai_family not supported");
    }
}


TEST(MethodChecking, RemoteHostResponse) {

    NetworkListener socket;
    socket.fetchInterfacesToConnect("localhost","65000", AF_INET6, 0);

    for (const auto& interface: socket.getInterfaces()) {
        EXPECT_EQ(interface.port, 65000);
        EXPECT_TRUE(interface.family == AF_INET6);
        EXPECT_TRUE(interface.socketType == SOCK_DGRAM || interface.socketType == SOCK_STREAM || interface.socketType == SOCK_RAW );
        EXPECT_TRUE(interface.ipString == "::1"); // localhost ipv6
    }
}


TEST(MethodChecking, RemoteHostResponseError) {

    NetworkListener socket;
    try {
        socket.fetchInterfacesToConnect("myata","65000", AF_INET6, 0);
    } catch (std::string error) {
        EXPECT_EQ(error, "The name resolution was not able to find any viable interfaces for the socket connection to myata on port: 65000 and IP version: 10.\nPrompting the error with status code: Temporary failure in name resolution");
    }
}
