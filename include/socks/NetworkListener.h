//
// Created by miguel on 2/13/26.
//

#ifndef SOCKS_ADDRESSINFO_H
#define SOCKS_ADDRESSINFO_H
#include <chrono>
#include <cstring>
#include <memory>
#include <netdb.h>
#include <string>
#include <vector>
#include <arpa/inet.h>

#include "AddressInfo.h"
#include "Exceptions.h"
#include "types.h"

class NetworkListener {

private:
    std::vector<AddressInfo> m_interfaces {};

    friend class L4Socket;

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
                const char* convResult = inet_ntop(ite->ai_family, &ipPtr->sin_addr, ipNameBuffer, ite->ai_addrlen);
                if (convResult == nullptr) {
                    throw ConversionToIPStringException();
                }
                interface.port = ntohs(ipPtr->sin_port);
            } else if (ite->ai_family == AF_INET6) {
                sockaddr_in6* ipPtr {};
                ipPtr = reinterpret_cast<sockaddr_in6*>(ite->ai_addr);
                const char* convResult = inet_ntop(ite->ai_family, &ipPtr->sin6_addr, ipNameBuffer, ite->ai_addrlen);
                if (convResult == nullptr) {
                    throw ConversionToIPStringException();
                }
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
            std::string error {};
            throw NameResolutionException(hostname,ipVersion,port,status);
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

        if ( status != 0 || result == nullptr) {
            std::string error {};
            throw NameResolutionException("localhost",ipVersion,port,status);
        }
        loadInterfaces(result);
    };

};

#endif //SOCKS_ADDRESSINFO_H