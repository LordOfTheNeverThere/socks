#ifndef EXTERNAL_INTERFACE_H
#define EXTERNAL_INTERFACE_H
#include <cstring>
#include <fstream>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <algorithm>

#include "GenericException.h"
#include "HostException.h"
#include "IPv4ToMACARPPacket.h"
#include "Tools.h"


class ExternalInterface{
    friend class InternalInterface;
private:
    std::string m_macVendor {};
    std::string m_ipAddress {};
    std::string m_macAddress {};
    std::string m_networkMask {};
    sa_family_t m_ipVersion{};

    void clearAttributes() {
        m_macVendor = "";
        m_ipAddress = "";
        m_macAddress = "";
        m_networkMask = "";
        m_ipVersion = 0;
    }

    std::string getOUI() {
        return m_macAddress.substr(0, 6);
    }

    void populateNameFromVendor() {
        std::ifstream ouiFile("../../resources/oui.txt");
        std::string lineBuffer {};
        std::string name {};
        std::string oui {};

        while (std::getline(ouiFile, lineBuffer)) {
            oui = lineBuffer.substr(0,6);
            if (oui == getOUI()) {
                name = lineBuffer.substr(7);
                break;
            }
        }
        m_macVendor = name;
        if (name.empty()) {
            m_macVendor = "Unknown Vendor";
        }
    }

public:

    ExternalInterface(const std::string& ipAddress, const sa_family_t& ipVersion, const std::string& networkMask,  const std::string& macAddress)
    :
    m_ipAddress {ipAddress},
    m_ipVersion {ipVersion},
    m_networkMask {networkMask}
    {setMacAddress(macAddress);}

    ExternalInterface()=default;

    [[nodiscard]] std::string getMacVendor() const {
        return m_macVendor;
    }

    [[nodiscard]] std::string getIPAddress() const {
        return m_ipAddress;
    }

    void setIPAddress(const std::string &ipString) {
        m_ipAddress = ipString;
    }

    [[nodiscard]] std::string getNetworkMask() const {
        return m_networkMask;
    }

    void setNetworkMask(const std::string &networkMask) {
        m_networkMask = networkMask;
    }

    sa_family_t getIPVersion() {
        return m_ipVersion;
    }

    void setIPVersion(const sa_family_t& ipVersion) {
        m_ipVersion = ipVersion;
    }

    [[nodiscard]] std::string getMacAddress() const {
        return m_macAddress;
    }

    void setMacAddress(const std::string &macAddress) {
        if (macAddress.size() != 12) {
            throw GenericException("Mac addresses must be 12 characters long");
        }
        m_macAddress = macAddress;
        std::transform(m_macAddress.begin(), m_macAddress.end(), m_macAddress.begin(), ::toupper);
        populateNameFromVendor();
    }

    void populateFromARPEchoReply(const uint8_t* reply, const std::string& ipString = "") {
        if (ipString.empty()) {
            IPv4ToMACARPPacket arpResponse {};
            std::memcpy(&arpResponse, reply + sizeof(ether_header), sizeof(IPv4ToMACARPPacket));
            char srcIPAddress[INET_ADDRSTRLEN] {};
            const char* convResult = inet_ntop(AF_INET, &arpResponse.srcIPv4, srcIPAddress,INET_ADDRSTRLEN);
            if (convResult == nullptr) {
                throw HostException("Conversion of the received packet's IP address was unsuccessful. \n Reason: \n" + std::system_category().message(errno));
            }
            setIPAddress(srcIPAddress);
        } else {
            setIPAddress(ipString);
        }

        ether_header ethernetHeaderResponse {};
        std::memcpy(&ethernetHeaderResponse, reply, sizeof(ether_header));
        setMacAddress(Tools::macToString(ethernetHeaderResponse.ether_shost));
    }


    bool belongsToSubnet(const std::string& ip) const {
        if (m_networkMask.empty()) {
            throw HostException("Cannot check if the interface with IP  " + m_ipAddress + " belongs to the same network as "
                + ip + " since the interface has no network mask defined.");
        }
        bool belongsToSubnet {false};

        if (m_ipVersion == AF_INET) {
            Int result {0};
            in_addr ipInBytes {};
            in_addr maskInBytes {};
            in_addr ipInterfaceInBytes{};

            result += inet_pton(m_ipVersion, ip.c_str(), &ipInBytes);
            result += inet_pton(m_ipVersion, m_networkMask.c_str(), &maskInBytes);
            result += inet_pton(m_ipVersion, m_ipAddress.c_str(), &ipInterfaceInBytes);

            if (result != 3) {
                throw HostException("It was not possible to convert the addresses into its binary form from string \nReason:\n " + std::system_category().message(errno));
            }

            belongsToSubnet = Tools::belongsToSubnetIPv4(ipInBytes.s_addr, ipInterfaceInBytes.s_addr, maskInBytes.s_addr);

        } else if (m_ipVersion == AF_INET6) {
            Int result {0};
            in6_addr ipInBytes {};
            in6_addr maskInBytes {};
            in6_addr ipInterfaceInBytes{};

            result += inet_pton(m_ipVersion, ip.c_str(), &ipInBytes);
            result += inet_pton(m_ipVersion, m_networkMask.c_str(), &maskInBytes);
            result += inet_pton(m_ipVersion, m_ipAddress.c_str(), &ipInterfaceInBytes);

            if (result != 3) {
                throw HostException("It was not possible to convert the addresses into its binary form from string \nReason:\n " + std::system_category().message(errno));
            }
            belongsToSubnet = Tools::belongsToSubnetIPv6(ipInBytes, ipInterfaceInBytes, maskInBytes);
        }
        return belongsToSubnet;
    }
};

inline std::ostream& operator<<(std::ostream& os, const ExternalInterface& host) {
    os << "MAC-Vendor: " << host.getMacVendor() << '\n' << "IP Address: " << host.getIPAddress() <<  '\n' << "MAC: " << host.getMacAddress() << '\n';
    return os;
}

inline bool operator==(const ExternalInterface& objLeft, const ExternalInterface& objRight) {
    return (objLeft.getMacAddress() == objRight.getMacAddress())
    && (objLeft.getIPAddress() == objRight.getIPAddress())
    && (objLeft.getNetworkMask() == objRight.getNetworkMask());
}
#endif //EXTERNAL_INTERFACE_H