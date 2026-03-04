//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_RAWSOCKET_H
#define SOCKS_RAWSOCKET_H
#include <system_error>
#include <unistd.h>
#include <chrono>
#include <cstring>

#include "RawSocketException.h"
#include "Socket.h"
#include <netinet/ip_icmp.h>
#include "Tools.h"


class RawSocket : private Socket {
public:
    RawSocket(Int& ipVersion, Int& protocol) {
        m_socket = socket(ipVersion, SOCK_RAW, protocol);
        if (m_socket == -1) {
            throw RawSocketException("The creation of the raw socket for protocol with ID: " + std::to_string(protocol) + " failed. \n Reason: \n" + std::system_category().message(errno));
        }
    }

    static void constructICMPPacket(uint8_t* packet, const size_t headerSize, const size_t nanosecsSize) {
        std::memset(packet, 0, headerSize + nanosecsSize);

        icmp icmpHeader {};
        // Build ICMP Header
        icmpHeader.icmp_type = ICMP_ECHO;
        icmpHeader.icmp_code = 0;
        icmpHeader.icmp_id = htons(getpid());
        icmpHeader.icmp_seq = 0;
        icmpHeader.icmp_cksum = 0; // Must be 0 for calculation

        uint64_t nanosecs = htobe64(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count()); // get nanosecs since Jan 1970
        //Load Buffer
        memcpy(packet, &icmpHeader, headerSize);
        memcpy(packet + headerSize, &nanosecs, nanosecsSize);

        const uint16_t checksum = htons(~Tools::add16BitOnesComplement(packet, headerSize + nanosecsSize)); // calculate one's complement of the 8bitOnesComplementSum
        memcpy(packet + 2, &checksum, sizeof(checksum)); //Populate the packet with the checksum value
    }

    void pinging(bool generateIPHeader = true) {
        const size_t headerSize = sizeof(icmp);
        const size_t nanosecsSize = sizeof(uint64_t);

        // create memory buffer to load the packet unto
        uint8_t packet[headerSize + nanosecsSize] = {};
        constructICMPPacket(packet, headerSize, nanosecsSize);
    }

};


#endif //SOCKS_RAWSOCKET_H