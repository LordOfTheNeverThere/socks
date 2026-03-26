#ifndef EXTERNAL_INTERFACE_H
#define EXTERNAL_INTERFACE_H
#include <cstring>
#include <fstream>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <algorithm>
#include <netinet/ip_icmp.h>
#include <chrono>

#include "Exceptions.h"
#include "IPv4Header.h"
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
    uint64_t m_timelapse {};    // In Nanosecond

    void clearAttributes() {
        m_macVendor = "";
        m_ipAddress = "";
        m_macAddress = "";
        m_networkMask = "";
        m_ipVersion = 0;
        m_timelapse = 0;
    }

    std::string getOUI() {
        return m_macAddress.substr(0, 6);
    }

    void populateNameFromVendor() {
        std::ifstream ouiFile("resources/oui.txt");
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

    void setTimelapse(const uint64_t& timelapse) {  // In Nanosecond
        m_timelapse = timelapse;
    }

    uint64_t getTimelapse() const {   // In Nanosecond
        return m_timelapse;
    }

    void setIPVersion(const sa_family_t& ipVersion) {
        m_ipVersion = ipVersion;
    }

    [[nodiscard]] std::string getMacAddress() const {
        return m_macAddress;
    }

    void setMacAddress(const std::string &macAddress) {
        if (macAddress.size() != 12) {
            throw WrongMACFormat(macAddress);
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
                throw ConversionToIPStringException();
            }
            setIPAddress(srcIPAddress);
        } else {
            setIPAddress(ipString);
        }

        ether_header ethernetHeaderResponse {};
        std::memcpy(&ethernetHeaderResponse, reply, sizeof(ether_header));
        setMacAddress(Tools::macToString(ethernetHeaderResponse.ether_shost));
        setIPVersion(AF_INET);
    }


    void populateFromICMPEchoReply(const uint8_t* reply) {
        uint8_t ipHeaderReceiveBuffer[sizeof(ip)] {};
        std::memcpy(ipHeaderReceiveBuffer, reply, sizeof(ip));
        IPv4Header ipHeaderReceive {ipHeaderReceiveBuffer};

        setIPAddress(ipHeaderReceive.getSourceStr());
        setIPVersion(ipHeaderReceive.getVersion());
        uint64_t timelapse {};
        std::memcpy(&timelapse, reply + sizeof(ip) + sizeof(icmphdr), sizeof(timelapse));
        uint64_t currNanosecs = std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count();

        setTimelapse(currNanosecs - be64toh(timelapse));
    }


    bool belongsToSubnetIPv6(const in6_addr& ipAddr) const {
        if (m_networkMask.empty()) {
            throw MissingMaskException(m_macAddress);
        }
        Int result {0};

        in6_addr maskInBytes {};
        in6_addr ipInterfaceInBytes{};

        result = inet_pton(m_ipVersion, m_networkMask.c_str(), &maskInBytes);
        if (result != 1) throw  ConversionToIPBinaryException(m_networkMask);
        result = inet_pton(m_ipVersion, m_ipAddress.c_str(), &ipInterfaceInBytes);
        if (result != 1) throw  ConversionToIPBinaryException(m_ipAddress);

        return Tools::belongsToSubnetIPv6(ipAddr, ipInterfaceInBytes, maskInBytes);

    }

    bool belongsToSubnetIPv4(const uint32_t ipInBits) const {
        if (m_networkMask.empty()) {
            throw MissingMaskException(m_macAddress);
        }

        Int result {0};

        in_addr maskInBytes {};
        in_addr ipInterfaceInBytes{};

        result = inet_pton(m_ipVersion, m_networkMask.c_str(), &maskInBytes);
        if (result != 1) throw  ConversionToIPBinaryException(m_networkMask);
        result = inet_pton(m_ipVersion, m_ipAddress.c_str(), &ipInterfaceInBytes);
        if (result != 1) throw  ConversionToIPBinaryException(m_ipAddress);

        return Tools::belongsToSubnetIPv4(ipInBits, ipInterfaceInBytes.s_addr, maskInBytes.s_addr);
    }


    bool belongsToSubnet(const std::string& ip) const {
        if (m_networkMask.empty()) {
            throw MissingMaskException(m_macAddress);
        }
        bool belongsToSubnet {false};

        if (m_ipVersion == AF_INET) {
            in_addr ipInBytes {};
            Int result = inet_pton(m_ipVersion, ip.c_str(), &ipInBytes);
            if (result != 1) {
                throw  ConversionToIPBinaryException(ip);
            }

            belongsToSubnet = belongsToSubnetIPv4(ipInBytes.s_addr);

        } else if (m_ipVersion == AF_INET6) {
            in6_addr ipInBytes {};
            Int result = inet_pton(m_ipVersion, ip.c_str(), &ipInBytes);
            if (result != 1) {
                throw  ConversionToIPBinaryException(ip);
            }

            belongsToSubnet = belongsToSubnetIPv6(ipInBytes);
        }
        return belongsToSubnet;
    }
};

inline std::ostream& operator<<(std::ostream& os, const ExternalInterface& host) {
    if (!host.getMacAddress().empty()) { // Local
        os << "MAC-Vendor: " << host.getMacVendor() << '\n' << "MAC: " << host.getMacAddress() << '\n';
    } else { // Non-Local
        os << "Ping Timelapse (ns): " << host.getTimelapse() << '\n';
    }
    os << "IP Address: " << host.getIPAddress() <<  '\n';
    return os;
}

inline bool operator==(const ExternalInterface& objLeft, const ExternalInterface& objRight) {
    return (objLeft.getMacAddress() == objRight.getMacAddress())
    && (objLeft.getIPAddress() == objRight.getIPAddress())
    && (objLeft.getNetworkMask() == objRight.getNetworkMask());
}
#endif //EXTERNAL_INTERFACE_H