//
// Created by miguel on 3/13/26.
//

#ifndef SOCKS_LOCALHOST_H
#define SOCKS_LOCALHOST_H
#include <net/if.h>
#include <ifaddrs.h>
#include <vector>
#include <map>

#include "InternalInterface.h"
#include "NameResolutionException.h"

class LocalHost{

private:
    std::vector<InternalInterface> m_interfaces {};

public:
    LocalHost(bool populate = false)  {
        if (populate) {
            getDataFromCurrentHost();
        }
    }

    std::vector<InternalInterface> getInterfaces() {
        return m_interfaces;
    }

    void clearInterfaces() {
        for (auto interface: m_interfaces) {
            interface.clearAttributes();
        }
    }

    void getDataFromCurrentHost() {

        clearInterfaces();
        ifaddrs *interfaceAddresses {};

        Int result = getifaddrs(&interfaceAddresses);
        if (result != 0) {
            throw NameResolutionException("The resolution of the machine's address, gateway, netmask and name was unsuccessful.\nReason:\n "
                + std::system_category().message(errno));
        }
        std::map<std::string, std::string> macAddresses {};

        // Get MAC Addresses and UP NON LOOPBACK interfaces
        for (ifaddrs *ptrToInterface = interfaceAddresses; ptrToInterface != nullptr; ptrToInterface = ptrToInterface->ifa_next) {
            if (ptrToInterface->ifa_addr !=nullptr
            && (ptrToInterface->ifa_flags & IFF_LOOPBACK) != IFF_LOOPBACK
            && (ptrToInterface->ifa_flags & IFF_UP) == IFF_UP
            && ptrToInterface->ifa_addr->sa_family == AF_PACKET) {

                std::string interfaceName = ptrToInterface->ifa_name;
                std::string macAddress {Tools::macToString(reinterpret_cast<sockaddr_ll*>(ptrToInterface->ifa_addr))};
                macAddresses.insert(std::pair<std::string, std::string> (interfaceName, macAddress));
            }
        }

        //Populate the Interfaces with both Layer 2 and 3 info
        for (ifaddrs *ptrToInterface = interfaceAddresses; ptrToInterface != nullptr; ptrToInterface = ptrToInterface->ifa_next) {

            sa_family_t ipVersion {ptrToInterface->ifa_addr->sa_family};
            auto iterator {macAddresses.find(ptrToInterface->ifa_name)};
            if ((ipVersion == AF_INET || ipVersion == AF_INET6) && iterator != macAddresses.end()) {

                InternalInterface interface {};
                interface.setInterfaceName(ptrToInterface->ifa_name);
                interface.setMacAddress(iterator->second);
                interface.setIPVersion(ipVersion);

                if (ipVersion == AF_INET) {
                    char ipAddress[INET_ADDRSTRLEN] {};
                    const char *ptrToResultIP = inet_ntop(
                        ipVersion, &reinterpret_cast<sockaddr_in *>(ptrToInterface->ifa_addr)->sin_addr, ipAddress,
                        sizeof(ipAddress));
                    if (ptrToResultIP == nullptr) {
                        continue;
                    }
                    interface.setIPAddress(ipAddress);

                    const char *ptrToResultMask = inet_ntop(ipVersion, &reinterpret_cast<sockaddr_in*>(ptrToInterface->ifa_netmask)->sin_addr, ipAddress, sizeof(ipAddress));
                    if (ptrToResultMask == nullptr) {
                        continue;
                    }
                    interface.setNetworkMask(ipAddress);

                } else if (ipVersion == AF_INET6) {

                    char ipAddress[INET6_ADDRSTRLEN] {};
                    const char *ptrToResultIP = inet_ntop(ipVersion, &reinterpret_cast<sockaddr_in6*>(ptrToInterface->ifa_addr)->sin6_addr, ipAddress, sizeof(ipAddress));
                    if (ptrToResultIP == nullptr) {
                        continue;
                    }
                    interface.setIPAddress(ipAddress);

                    const char *ptrToResultMask = inet_ntop(ipVersion, &reinterpret_cast<sockaddr_in6*>(ptrToInterface->ifa_netmask)->sin6_addr, ipAddress, sizeof(ipAddress));
                    if (ptrToResultMask == nullptr) {
                        continue;
                    }
                    interface.setNetworkMask(ipAddress);
                }
                m_interfaces.push_back(interface);
            }
        }
        freeifaddrs(interfaceAddresses);
        if (m_interfaces.empty()) {
            throw (GenericException("It was not possible to populate the data"));
        }
    }

    InternalInterface getInterfaceFromSubnet(const std::string& ip, const sa_family_t& ipVersion) {
        for (auto interface: m_interfaces) {
            if (ipVersion == interface.getIPVersion() && interface.belongsToSubnet(ip)) {
                return interface;
            }
        }
        throw HostException("No interfaces connect to the network where " + ip + "is.");
    }
};


#endif //SOCKS_LOCALHOST_H