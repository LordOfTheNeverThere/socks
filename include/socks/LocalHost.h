//
// Created by miguel on 3/13/26.
//

#ifndef SOCKS_LOCALHOST_H
#define SOCKS_LOCALHOST_H
#include "Host.h"

class LocalHost : public Host{

private:
    std::string m_interfaceName {"Unknown"};
public:
    LocalHost(bool populate = false) {
        if (populate) {
            getDataFromCurrentHost();
        }
    }


    void getDataFromCurrentHost(Int ipVersion = AF_INET, std::string name = "") {

        clearAttributes();
        ifaddrs *interfaceAddresses {};

        Int result = getifaddrs(&interfaceAddresses);
        if (result != 0) {
            throw NameResolutionException("The resolution of the machine's address, gateway, netmask and name was unsuccessful.\nReason:\n "
                + std::system_category().message(errno));
        }
        std::map<std::string, std::string> macAddresses {};

        for (ifaddrs *ptrToInterface = interfaceAddresses; ptrToInterface != nullptr; ptrToInterface = ptrToInterface->ifa_next) {
            if (ptrToInterface->ifa_addr !=nullptr && (ptrToInterface->ifa_flags & IFF_LOOPBACK) != IFF_LOOPBACK && (ptrToInterface->ifa_flags & IFF_UP) == IFF_UP
            && ptrToInterface->ifa_addr->sa_family == AF_PACKET && (name.empty() || strcmp(name.c_str(), ptrToInterface->ifa_name) == 0)) {

                std::string interfaceName = ptrToInterface->ifa_name;
                std::string macAddress {Tools::macToString(reinterpret_cast<sockaddr_ll*>(ptrToInterface->ifa_addr))};
                macAddresses.insert(std::pair<std::string, std::string> (interfaceName, macAddress));
            }
        }

        if (macAddresses.size() == 1) {
            name = macAddresses.begin()->first;
        }

        for (ifaddrs *ptrToInterface = interfaceAddresses; ptrToInterface != nullptr && !isObjectPopulated() ; ptrToInterface = ptrToInterface->ifa_next) {
            if (ptrToInterface->ifa_addr != nullptr && ptrToInterface->ifa_addr->sa_family == ipVersion && ptrToInterface->ifa_netmask != nullptr
                && (ptrToInterface->ifa_flags & IFF_LOOPBACK) != IFF_LOOPBACK && (ptrToInterface->ifa_flags & IFF_UP) == IFF_UP) {

                if (!name.empty() && std::strcmp(name.c_str(), ptrToInterface->ifa_name) != 0) {
                    continue;
                } else {
                    setMacAddress(macAddresses.at(ptrToInterface->ifa_name));
                    m_interfaceName = ptrToInterface->ifa_name;
                }

                if (ipVersion == AF_INET) {
                    char ipAddress[INET_ADDRSTRLEN] {};

                    const char *ptrToResultIP = inet_ntop(
                        ipVersion, &reinterpret_cast<sockaddr_in *>(ptrToInterface->ifa_addr)->sin_addr, ipAddress,
                        sizeof(ipAddress));
                    if (ptrToResultIP == nullptr) {
                        continue;
                    }
                    m_ipAddress = ipAddress;

                    const char *ptrToResultMask = inet_ntop(ipVersion, &reinterpret_cast<sockaddr_in*>(ptrToInterface->ifa_netmask)->sin_addr, ipAddress, sizeof(ipAddress));
                    if (ptrToResultMask == nullptr) {
                        continue;
                    }
                    m_networkMask = ipAddress;

                } else if (ipVersion == AF_INET6) {

                    char ipAddress[INET6_ADDRSTRLEN] {};
                    const char *ptrToResultIP = inet_ntop(ipVersion, &reinterpret_cast<sockaddr_in6*>(ptrToInterface->ifa_addr)->sin6_addr, ipAddress, sizeof(ipAddress));
                    if (ptrToResultIP == nullptr) {
                        continue;
                    }
                    m_ipAddress = ipAddress;

                    const char *ptrToResultMask = inet_ntop(ipVersion, &reinterpret_cast<sockaddr_in6*>(ptrToInterface->ifa_netmask)->sin6_addr, ipAddress, sizeof(ipAddress));
                    if (ptrToResultMask == nullptr) {
                        continue;
                    }
                    m_networkMask = ipAddress;
                } else {

                    freeifaddrs(interfaceAddresses);
                    throw GenericException("The ip version supplied is not allowed, try AF_INET or AF_INET6");
                }
            }
        }
        freeifaddrs(interfaceAddresses);
        if (!isObjectPopulated()) {
            throw (GenericException("It was not possible to populate the data"));
        }
    }


    void clearAttributes() {
        m_interfaceName = "";
        Host::clearAttributes();
    }
};


#endif //SOCKS_LOCALHOST_H