//
// Created by miguel on 2/13/26.
//

#ifndef SOCKS_ADDRESSINFO_H
#define SOCKS_ADDRESSINFO_H
#include <cstring>
#include <iostream>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ostream>
#include <signal.h>
#include <string>
#include <system_error>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>
#include <bits/sigaction.h>
#include <sys/wait.h>

#include "AddressInfo.h"
#include "ClientSocketException.h"
#include "NameResolutionException.h"
#include "ServerSocketException.h"
#include "types.h"
#include "gtest/gtest_prod.h"

class NetworkListener {

private:
    std::vector<AddressInfo> m_interfaces {};
    Int m_socket {-1};

    FRIEND_TEST(MethodChecking, createSocketAndBind);
    FRIEND_TEST(MethodChecking, clientAndServerMock);

    void loadInterfaces(addrinfo *result) {

        m_interfaces.clear();
        for (addrinfo* ite = result; ite != nullptr; ite = ite->ai_next) {
            AddressInfo interface {};
            std::memcpy(&interface.addr, ite->ai_addr, ite->ai_addrlen);
            interface.family = ite->ai_family;
            interface.protocol = ite->ai_protocol;
            interface.socketType = ite->ai_socktype;
            interface.ipLength = ite->ai_addrlen;

            char ipNameBuffer[ite->ai_family];


            if (ite->ai_family == AF_INET) {
                sockaddr_in* ipPtr {};
                ipPtr = reinterpret_cast<sockaddr_in*>(ite->ai_addr);
                inet_ntop(ite->ai_family, &ipPtr->sin_addr, ipNameBuffer, ite->ai_addrlen);
                interface.port = ntohs(ipPtr->sin_port);
            } else if (ite->ai_family == AF_INET6) {
                sockaddr_in6* ipPtr {};
                ipPtr = reinterpret_cast<sockaddr_in6*>(ite->ai_addr);
                inet_ntop(ite->ai_family, &ipPtr->sin6_addr, ipNameBuffer, ite->ai_addrlen);
                interface.port = ntohs(ipPtr->sin6_port);
            } else {
                std::string ipNameBuffer = "Undefined";
                interface.port = -1;
            }
            interface.ipString = ipNameBuffer;

            m_interfaces.push_back(interface);
        }
        freeaddrinfo(result); // This linked list is no longer required - Memory leak avoidance
    }

    bool bindSocketToInterface(AddressInfo& interface) {
        bool continues = false;
        Int yes = 1;

        Int setSocketOption = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(Int));


        if ( setSocketOption == -1) {
            exit(1);
        }

        // setSocketOption = setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(Int));
        //
        // if ( setSocketOption == -1) {
        //     exit(1);
        // }
        // Allows usage of ports and addresses in less than 2 mins after their closure.

        Int binding = bind(m_socket, reinterpret_cast<sockaddr*>(&interface.addr), interface.ipLength);
        if (binding == -1) {
            closeSocket();
            continues = true;
        }
        return continues;
    }


    bool connectSocketToInterface(AddressInfo & interface) {
        bool continues = false;
        Int connecting = connect(m_socket, reinterpret_cast<sockaddr*>(&interface.addr), interface.ipLength);
        if (connecting == -1) {
            closeSocket();
            continues = true;
        }
        return continues;
    }

    static void sigChildHandler(Int s){
        (void)s; // quiet unused variable warning

        // waitpid() might overwrite errno, so we save and restore it:
        const Int saved = errno;

        while(waitpid(-1, nullptr, WNOHANG) > 0) {}

        errno = saved;
    }

    void closeSocket() {
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1; // Prevent double-close or closing wrong FDs
        }
    }
public:

    [[nodiscard]] std::vector<AddressInfo> getInterfaces() const {
        return m_interfaces;
    }


    void fetchInterfacesToConnect(const std::string& hostname,const std::string& port, const Int& ipVersion = 0, const Int& socketType = 0) {

        addrinfo hints {};
        hints.ai_family = ipVersion;
        hints.ai_socktype = socketType;
        addrinfo *result = nullptr;
        const Int status = getaddrinfo(hostname.c_str(), port.c_str(), &hints, &result);

        if ( status != 0 || result == nullptr) {
            std::string error {"The name resolution was not able to find any viable interfaces for the socket connection to " + hostname + " on port: " + port + " and IP version: " + std::to_string(ipVersion) + ".\n" + "Prompting the error with status code: " + gai_strerror(status)};
            throw NameResolutionException(error);
        }
        loadInterfaces(result);
    };

    void fetchInterfacesToBind( const std::string& port, const Int& ipVersion = 0, const Int& socketType = 0) {

        addrinfo hints {};
        hints.ai_flags = AI_PASSIVE;
        hints.ai_family = ipVersion;
        hints.ai_socktype = socketType;
        addrinfo *result = nullptr;
        const Int status = getaddrinfo(nullptr, port.c_str(), &hints, &result);

        if ( status != 0 || result == nullptr) { // Resolve localhost's name and return available interfaces
            std::string error {"The name resolution was not able to find any viable interfaces for the socket connection to localhost on port: " + port + " and IP version: " + std::to_string(ipVersion) + ".\n" + "Prompting the error with status code: " + gai_strerror(status)};
            throw NameResolutionException(error);
        }
        loadInterfaces(result);
    };


    void createSocket(const bool isServer) {

        for (AddressInfo &interface: m_interfaces) {

            m_socket = socket(interface.family, interface.socketType, interface.protocol);

            if (m_socket == -1) {
                perror("Got an error while creating a socket");
                continue;
            }

            if (isServer) {
                if (bindSocketToInterface(interface)) {
                    const std::string error = "The binding of the socket to port: " + std::to_string(interface.port) + " failed. \n Reason: \n" + std::system_category().message(errno);
                    throw ServerSocketException(error);
                };
            }
            else {
                if (connectSocketToInterface(interface)) {
                    const std::string error = "The connection of the socket to IP and port: " + interface.ipString + ":" + std::to_string(interface.port) + " on the server failed. \n Reason: \n" + std::system_category().message(errno);
                    throw ClientSocketException(error);
                };
            }
            return;
        }
        if (isServer) {
            throw ServerSocketException("It was not possible to bind any interface to the socket.");
        }
        else {
            throw ClientSocketException("It was not possible to connect any interface to the socket.");
        }
    }

    void serverSends(void* msgToSend, size_t msgSize, Int connectionQueueNumber = 7, bool multiConnection = true) {
        Int listening = listen(m_socket, connectionQueueNumber);
        if (listening == -1) {
            throw ServerSocketException("Could not make a listening socket for the server. \n Reason: \n" + std::system_category().message(errno));
        }

        sockaddr_storage clientAddress {};
        socklen_t sizeClientAddress = sizeof(clientAddress);

        if (multiConnection) {
            struct sigaction sa {};
            sa.sa_handler = sigChildHandler;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = SA_RESTART;
            if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
                throw ServerSocketException("Could not reap zombie processes of previously closed connections");
            }
            while (true) {
                Int connectedSocket = accept(m_socket, reinterpret_cast<sockaddr*>(&clientAddress), &sizeClientAddress);
                if (connectedSocket == -1) {
                    std::string ipNameBuffer {""};
                    std::cerr << "It was not possible to accept the connection to the socket from: " ;
                    if (reinterpret_cast<sockaddr*>(&clientAddress)->sa_family == AF_INET) {
                        inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&clientAddress)->sin_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in));
                    } else {
                        inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(&clientAddress)->sin6_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in6));
                    }
                    std::cerr << ipNameBuffer.data();
                    continue;
                }
                if (!fork()) { // this is the child process
                    closeSocket(); // child doesn't need the listener
                    if (send(connectedSocket, msgToSend, msgSize, 0) == -1)
                        std::cerr << "Error sending. \n Reason: \n " + std::system_category().message(errno);
                    close(connectedSocket);
                    exit(0);
                }
                close(connectedSocket);  // parent doesn't need this
            }
        } else {
            Int connectedSocket = accept(m_socket, reinterpret_cast<sockaddr*>(&clientAddress), &sizeClientAddress);
            if (connectedSocket == -1) {
                std::string ipNameBuffer {""};
                if (reinterpret_cast<sockaddr*>(&clientAddress)->sa_family == AF_INET) {
                    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&clientAddress)->sin_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in));
                } else {
                    inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(&clientAddress)->sin6_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in6));
                }
                throw ServerSocketException("It was not possible to accept the connection to the socket from: " + ipNameBuffer);
            }
            closeSocket(); // Connection reached no more listening required
            if (send(connectedSocket, msgToSend, msgSize, 0) == -1) {
                throw ServerSocketException("Error sending. \n Reason: \n " + std::system_category().message(errno));
            }
            close(connectedSocket); // message was sent no more sending required.
        }
    }

    Int clientCollects(void* msgReceived, const size_t msgSize) const {
        const Int bytesReceived = static_cast<Int>(recv(m_socket, msgReceived, msgSize, 0));
        if (bytesReceived == -1) {
            throw ClientSocketException("It was not possible for the client to collect the message from the server. \n Reason : \n " + std::system_category().message(errno));
        }
        return bytesReceived;
    }
};

#endif //SOCKS_ADDRESSINFO_H