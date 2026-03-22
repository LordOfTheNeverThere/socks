#include "socks/NetworkListener.h"

#include <gtest/gtest.h>

#include "socks/L4Socket.h"


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
    EXPECT_THROW({
    try {
        socket.fetchInterfacesToBind("8080", 7, 0);
    } catch( const NameResolutionException& e ) {
        EXPECT_STREQ("The name resolution was not able to find any viable interfaces for the socket connection to localhost on port: 8080 and IP version: 7."
                     "\nPrompting the error with status code: ai_family not supported"
            , e.what());
        throw; // Re-throw so EXPECT_THROW sees it
    }
}, NameResolutionException);
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

    EXPECT_THROW({
        try {
            socket.fetchInterfacesToConnect("myata","65000", AF_INET6, 0);
        } catch( const NameResolutionException& e ) {
            EXPECT_STREQ("The name resolution was not able to find any viable interfaces for the socket connection to myata on port: 65000 and IP version: 10."
                         "\nPrompting the error with status code: Temporary failure in name resolution"
                , e.what());
            throw; // Re-throw so EXPECT_THROW sees it
        }
    }, NameResolutionException);
}


TEST(MethodChecking, createSocketAndBind) {
    NetworkListener listener;
    listener.fetchInterfacesToBind("8080", 0, 0);
    std::string error {""};
    try {
        L4Socket socket =  L4Socket(listener, true);
        EXPECT_NE(socket.m_socket, -1);
    } catch (std::runtime_error& caught) {
        error = caught.what();
    }
    EXPECT_EQ(error,"");
}




//Single Connection
TEST(MethodChecking, clientAndServerMock) {
    NetworkListener client;
    client.fetchInterfacesToConnect("localhost", "8088", AF_INET, 0);
    NetworkListener server;
    server.fetchInterfacesToBind("8088", AF_INET, 0);
    char* outgoing = "PING";
    char incoming[5] = {0}; // 4 chars + 1 for null terminator

    int pipefds[2];
    pipe(pipefds); // [0] is read, [1] is write
    if (!fork()) {
        close(pipefds[0]);

        L4Socket serverSocket =  L4Socket(server, true);
        serverSocket.send(outgoing, 5, 1, false);

        write(pipefds[1], &serverSocket.m_socket, sizeof(serverSocket.m_socket));
        _exit(0);
    }
    usleep(50000);
    L4Socket clientSocket =  L4Socket(client, false);
    clientSocket.receive(incoming, 5);
    EXPECT_TRUE(!strcmp(incoming, outgoing));
    clientSocket.closeSocket();
    EXPECT_EQ(clientSocket.m_socket, -1);
    Int serverSocket {0};
    read(pipefds[0], &serverSocket, sizeof(serverSocket));
    EXPECT_EQ(serverSocket, -1);
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
        try {
            L4Socket serverSocket =  L4Socket(server, true);
            serverSocket.send(outgoing, 5, 5, true, connectionTimeout);

        } catch (std::runtime_error& caught) {
            std::cerr << caught.what() << "\n";
        }
        exit(0);
    }

    int pipefds[2];
    pipe(pipefds); // [0] is read, [1] is write

    if (!fork()) {
        close(pipefds[0]); // Close read end
        L4Socket clientOneSocket = L4Socket(clientOne, false);
        char incoming[5] = {0};
        clientOneSocket.receive(incoming, 5);

        bool success = !strcmp(incoming, outgoing);
        write(pipefds[1], &success, sizeof(success));

        clientOneSocket.closeSocket();
        _exit(0);
    }

    close(pipefds[1]); // Close write end
    bool childSuccess = false;
    read(pipefds[0], &childSuccess, sizeof(childSuccess));
    EXPECT_TRUE(childSuccess);

    char incoming[5] = {0};
    L4Socket clientTwoSocket = L4Socket(clientTwo, false);
    clientTwoSocket.receive(incoming, 5);
    EXPECT_TRUE(!strcmp(incoming, outgoing));

    clientTwoSocket.closeSocket();
    EXPECT_EQ(clientTwoSocket.m_socket, -1);
    sleep(connectionTimeout - 2);
}