#ifndef INTERNAL_INTERFACE_H
#define INTERNAL_INTERFACE_H

#include <memory>
#include <string>

#include "ExternalInterface.h"

class InternalInterface : public ExternalInterface {
    friend class ExternalInterface;
private:
    std::string m_interfaceName {};


    bool isObjectPopulated() const {
        return !(m_macVendor.empty() || m_ipAddress.empty() || m_networkMask.empty() || m_macAddress.empty() || m_interfaceName.empty());
    }



public:

    InternalInterface(const std::string& ipAddress, const std::string& networkMask, const std::string& macAddress, const sa_family_t& ipVersion, const std::string& interfaceName = "")
    : m_interfaceName{interfaceName},
    ExternalInterface(ipAddress, ipVersion, networkMask, macAddress)
    {}

    InternalInterface()=default;

    std::string getInterfaceName() const {
        return m_interfaceName;
    }


    void setInterfaceName(const std::string& interfaceName) {
        m_interfaceName = interfaceName;
    }

    void clearAttributes() {
        m_interfaceName = "";
        ExternalInterface::clearAttributes();
    }

};


inline bool operator==(InternalInterface& objLeft, InternalInterface& objRight) {
    return operator==(static_cast<ExternalInterface>(objLeft), static_cast<ExternalInterface>(objRight))
    && (objLeft.getInterfaceName() == objRight.getInterfaceName());
}
#endif //INTERNAL_INTERFACE_H