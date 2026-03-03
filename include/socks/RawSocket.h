//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_RAWSOCKET_H
#define SOCKS_RAWSOCKET_H
#include <system_error>
#include <unistd.h>

#include "RawSocketException.h"
#include "Socket.h"
#include <netinet/ip_icmp.h>

class RawSocket : public Socket {
public:
    RawSocket(Int& ipVersion, Int& protocol) {
        m_socket = socket(ipVersion, SOCK_RAW, protocol);
        if (m_socket == -1) {
            throw RawSocketException("The creation of the raw socket for protocol with ID: " + std::to_string(protocol) + " failed. \n Reason: \n" + std::system_category().message(errno));
        }
    }
    void pinging(bool generateHeader = true) {

        icmp icmpHeader {};
        // Build ICMP Header
        icmpHeader.icmp_type = ICMP_ECHO;
        icmpHeader.icmp_code = 0;
        icmpHeader.icmp_id = getpid();
        icmpHeader.icmp_seq = 0;
        icmpHeader.icmp_cksum = 0; // Must be 0 for calculation
    }

};


#endif //SOCKS_RAWSOCKET_H