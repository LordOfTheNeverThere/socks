#ifndef SOCKS_HOST_H
#define SOCKS_HOST_H
#include <algorithm>
#include <cstring>
#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <ifaddrs.h>
#include <map>
#include <system_error>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>

#include "NameResolutionException.h"
#include "Tools.h"
#include "types.h"

class Host {
    friend class LocalHost;
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
        m_name = name;
        if (name.empty()) {
            m_name = "Unknown Vendor";
        }
    }

public:

    Host(const std::string& name, const std::string& ipAddress, const std::string& networkMask, const std::string& macAddress)
    :
    m_name {name}, m_ipAddress {ipAddress},
    m_networkMask {networkMask}, m_macAddress {macAddress}
    {}

    Host()=default;

    [[nodiscard]] std::string getName() const {
        return m_name;
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
        if (m_mac_address.size() != 12) {
            throw GenericException("Mac addresses must be 12 characters long");
        }
        m_macAddress = m_mac_address;
        std::transform(m_macAddress.begin(), m_macAddress.end(), m_macAddress.begin(), ::toupper);
        populateNameFromVendor();
    }



    void populateFromARPEchoReply(uint8_t* reply) {


    }
};
#endif //SOCKS_HOST_H