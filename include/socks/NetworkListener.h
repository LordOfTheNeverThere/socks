//
// Created by miguel on 2/13/26.
//

#ifndef SOCKS_ADDRESSINFO_H
#define SOCKS_ADDRESSINFO_H
#include <cstring>
#include <memory>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>

#include "AddressInfo.h"
#include "types.h"

class NetworkListener {

private:
    std::vector<AddressInfo> m_interfaces {};
    Int m_socket {};

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

    bool bindSocketToInterface(AddressInfo& interface) const {
        bool continues = false;
        bool yes = true;

        Int setSocketOption = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int));
        // Allows usage of ports in less than 2 mins after their closure.
        if ( setSocketOption == -1) {
            perror("Got an error while clearing previously used ports");
            exit(1);
        }

        Int binding = bind(m_socket, reinterpret_cast<sockaddr*>(&interface.addr), interface.ipLength);
        if (binding == -1) {
            close(m_socket);
            perror(("Got an error while binding a socket to the interface's port :" + std::to_string(interface.port)).c_str());
            continues = true;
        }
        return continues;
    }

    bool connectSocketToInterface(AddressInfo & interface) const {
        bool continues = false;
        Int connecting = connect(m_socket, reinterpret_cast<sockaddr*>(&interface.addr), interface.ipLength);
        if (connecting == -1) {
            perror(("Got an error while connecting a socket to interface with IP and Port " + interface.ipString + ":" + std::to_string(interface.port)).c_str());
            close(m_socket);
            continues = true;
        }
        return continues;
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
            throw error;
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
            throw error;
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
                if (bindSocketToInterface(interface)) continue;
            }
            else {
                if (connectSocketToInterface(interface)) continue;
            }
            return;
        }
        throw "It was not possible to bind any interface to the socket.";
    }

    void socketSending(){}


    void socketListening(const bool isUnicast) {
    }

};






#endif //SOCKS_ADDRESSINFO_H