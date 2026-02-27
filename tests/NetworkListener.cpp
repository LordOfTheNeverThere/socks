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


TEST(ErrorChecking, LocalhostResponseError) {
    NetworkListener socket;
    try {
        socket.fetchInterfacesToBind("8080", 7, 0);
    } catch (GenericException& error) {
        EXPECT_EQ(error.what(), "The name resolution was not able to find any viable interfaces for the socket connection to localhost on port: 8080 and IP version: 7.\nPrompting the error with status code: ai_family not supported");
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


TEST(ErrorChecking, RemoteHostResponseError) {

    NetworkListener socket;
    try {
        socket.fetchInterfacesToConnect("myata","65000", AF_INET6, 0);
    } catch (GenericException& error) {
        EXPECT_EQ(error.what(), "The name resolution was not able to find any viable interfaces for the socket connection to myata on port: 65000 and IP version: 10.\nPrompting the error with status code: Temporary failure in name resolution");
    }
}


TEST(MethodChecking, createSocketAndBind) {
    NetworkListener listener;
    listener.fetchInterfacesToBind("8080", 0, 0);
    std::string error {""};
    try {
        listener.createSocket(true);
    } catch (GenericException& caught) {
        error = caught.what();
    }
    EXPECT_EQ(error,"");
    EXPECT_NE(listener.m_socket, -1);
}




//Single Connection
TEST(MethodChecking, clientAndServerMock) {
    NetworkListener client;
    client.fetchInterfacesToConnect("localhost", "8088", AF_INET, 0);
    NetworkListener server;
    server.fetchInterfacesToBind("8088", AF_INET, 0);
    char* outgoing = "PING";
    char incoming[5] = {0}; // 4 chars + 1 for null terminator

    if (!fork()) {
        server.createSocket(true);
        server.serverSends(outgoing, 5, 1, false);
        _exit(0);
    }
    usleep(50000);
    client.createSocket(false);
    client.clientCollects(incoming, 5);
    EXPECT_TRUE(!strcmp(incoming, outgoing));
    client.closeSocket();
    EXPECT_EQ(client.m_socket, -1);
    EXPECT_EQ(server.m_socket, -1);
}


// Multiple connections
TEST(MethodChecking, twoClientAndServerMock) {

    Int connectionTimeout = 5;

    NetworkListener clientOne;
    clientOne.fetchInterfacesToConnect("localhost", "8088", AF_INET, 0);
    NetworkListener clientTwo;
    clientTwo.fetchInterfacesToConnect("localhost", "8088", AF_INET, 0);
    NetworkListener server;
    server.fetchInterfacesToBind("8088", AF_INET, 0);

    char* outgoing = "PING";

    if (!fork()) {
        server.createSocket(true);
        try {
            server.serverSends(outgoing, 5, 5, true, connectionTimeout);

        } catch (GenericException caught) {
            std::cerr << caught.what() << "\n";
        }

        exit(0);
    }

    int pipefds[2];
    pipe(pipefds); // [0] is read, [1] is write
    if (!fork()) {
        close(pipefds[0]); // Close read end
        clientOne.createSocket(false);
        char incoming[5] = {0};
        clientOne.clientCollects(incoming, 5);

        bool success = !strcmp(incoming, outgoing);
        write(pipefds[1], &success, sizeof(success));

        server.closeSocket();
        _exit(0);
    }

    close(pipefds[1]); // Close write end
    bool childSuccess = false;
    read(pipefds[0], &childSuccess, sizeof(childSuccess));
    EXPECT_TRUE(childSuccess);

    char incoming[5] = {0};
    clientTwo.createSocket(false);
    clientTwo.clientCollects(incoming, 5);
    EXPECT_TRUE(!strcmp(incoming, outgoing));

    clientOne.closeSocket();
    clientTwo.closeSocket();
    EXPECT_EQ(clientOne.m_socket, -1);
    EXPECT_EQ(server.m_socket, -1);
    sleep(connectionTimeout - 2);
}