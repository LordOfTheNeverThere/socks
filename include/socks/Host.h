//
// Created by miguel on 3/9/26.
//

#ifndef SOCKS_HOST_H
#define SOCKS_HOST_H
#include <cstring>
#include <memory>
#include <string>
#include <ifaddrs.h>
#include <iostream>
#include <map>
#include <system_error>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>

#include "NameResolutionException.h"
#include "Tools.h"
#include "types.h"

class Host {
private:
    std::string m_name {};
    std::string m_ipAddress {};
    std::string m_networkMask {};
    std::string m_macAddress {};

    bool isObjectPopulated() const {
        return !(m_name.empty() || m_ipAddress.empty() || m_networkMask.empty() || m_macAddress.empty());
    }
    void clearAttributes() {
        m_name = "";
        m_ipAddress = "";
        m_networkMask = "";
        m_macAddress = "";
    }

public:
    Host(const std::string& name, const std::string& ipAddress, const std::string& networkMask, const std::string& macAddress)
    :
    m_name {name}, m_ipAddress {ipAddress},
    m_networkMask {networkMask}, m_macAddress {macAddress}
    {}

    Host(bool populate = false) {
        if (populate) {
            getDataFromCurrentHost();
        }
    }

    [[nodiscard]] std::string getName() const {
        return m_name;
    }

    void setName(const std::string &m_name) {
        this->m_name = m_name;
    }

    [[nodiscard]] std::string getIPAddress() const {
        return m_ipAddress;
    }

    void setIPAddress(const std::string &m_ip_address) {
        m_ipAddress = m_ip_address;
    }

    [[nodiscard]] std::string getNetworkMask() const {
        return m_networkMask;
    }

    void setNetworkMask(const std::string &m_network_mask) {
        m_networkMask = m_network_mask;
    }

    [[nodiscard]] std::string getMacAddress() const {
        return m_macAddress;
    }

    void setMacAddress(const std::string &m_mac_address) {
        m_macAddress = m_mac_address;
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

                if (name.empty()) {
                    m_name = ptrToInterface->ifa_name;
                } else if (std::strcmp(name.c_str(), ptrToInterface->ifa_name) == 0) {
                    m_name = name;
                } else {
                    continue;
                }
                m_macAddress = macAddresses.at(m_name);

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
};
#endif //SOCKS_HOST_H