//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_RAWSOCKET_H
#define SOCKS_RAWSOCKET_H
#include <array>
#include <system_error>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include "RawSocketException.h"
#include "Socket.h"
#include <netinet/ip_icmp.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

#include "IPv4Header.h"
#include "Tools.h"
#include "IPv4ToMACARPPacket.h"
#include "LocalHost.h"


class RawSocket : private Socket {

private:
    Int m_ipVersion {0};
    Int m_protocol {0};
    IPv4Header m_ipHeader {IPv4Header()};
public:
    RawSocket(const Int& ipVersion, const Int& protocol) :  m_protocol(protocol) {

        if (ipVersion != AF_INET && ipVersion != AF_INET6 && ipVersion != AF_PACKET) {
            throw RawSocketException("The family type with ID: " + std::to_string(ipVersion) + " is not supported.");
        } else {
            m_ipVersion = ipVersion;
        }

        m_socket = socket(ipVersion, SOCK_RAW, protocol);
        if (m_socket == -1) {
            throw RawSocketException("The creation of the raw socket for protocol with ID: " + std::to_string(protocol) + " failed. \n Reason: \n" + std::system_category().message(errno));
        }
    }

    void setIPHeader(IPv4Header& ipHeader) {
        if (ipHeader.header != nullptr) {
            ipHeader.setProtocol(IPPROTO_ICMP);
            m_ipHeader = ipHeader;
            Int one = 1;
            Int socketOpt = setsockopt(m_socket,IPPROTO_IP,IP_HDRINCL, &one, sizeof(one));
            m_ipVersion = AF_INET;
            if (socketOpt == -1) {
                throw RawSocketException("The setting of the socket option IP_HDRINCL failed.\nReason:\n" + std::system_category().message(errno));
            }
        }
    }

    [[nodiscard]] bool autogenerateIPHeader() const {
        return m_ipHeader.header == nullptr;
    }

    size_t constructICMPPacketWithIPHeader(uint8_t* packet, const size_t headerSize, const size_t nanosecsSize, const uint16_t& seqNum, const uint16_t& processIDNum)  {

        size_t ipHeaderSize {m_ipHeader.getHeaderLenghtInBytes()};
        m_ipHeader.setTotalLength(ipHeaderSize + headerSize + nanosecsSize);
        std::memset(packet, 0, ipHeaderSize + headerSize + nanosecsSize);
        std::memcpy(packet, m_ipHeader.header, ipHeaderSize);

        return ipHeaderSize + constructICMPPacket(packet + ipHeaderSize, headerSize, nanosecsSize, seqNum, processIDNum);
    }

    static size_t constructICMPPacket(uint8_t* packet, const size_t headerSize, const size_t nanosecsSize, const uint16_t& seqNum, const uint16_t& processIDNum) {
        std::memset(packet, 0, headerSize + nanosecsSize);
        icmp icmpHeader {};
        // Build ICMP Header
        icmpHeader.icmp_type = ICMP_ECHO;
        icmpHeader.icmp_code = 0;
        icmpHeader.icmp_id = htons(processIDNum);
        icmpHeader.icmp_seq = htons(seqNum);
        icmpHeader.icmp_cksum = 0; // Must be 0 for calculation

        uint64_t nanosecs = htobe64(std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch().count()); // get nanosecs since Jan 1970
        //Load Buffer
        memcpy(packet, &icmpHeader, headerSize);
        memcpy(packet + headerSize, &nanosecs, nanosecsSize);

        const uint16_t checksum = htons(~Tools::add16BitOnesComplement(packet, headerSize + nanosecsSize)); // calculate one's complement of the 8bitOnesComplementSum
        memcpy(packet + 2, &checksum, sizeof(checksum)); //Populate the packet with the checksum value

        return headerSize + nanosecsSize;
    }


    size_t sendPing(const std::string& destIP, const uint16_t& seqNum = 0, const uint16_t& processIDNum = 0){


        sockaddr_storage destination {};
        destination.ss_family = m_ipVersion;

        Int conversionResult {};
        if (m_ipVersion == AF_INET) {
            conversionResult = inet_pton(m_ipVersion, destIP.c_str(), &(reinterpret_cast<sockaddr_in*>(&destination)->sin_addr));
        } else if (m_ipVersion == AF_INET6) {
            conversionResult = inet_pton(m_ipVersion, destIP.c_str(), &(reinterpret_cast<sockaddr_in6*>(&destination)->sin6_addr));
        }


        const size_t headerSize = sizeof(icmphdr); //use icmphdr instead of icmp for requests, icmp has part of the IP header which caused the host to reply with an error, since this is request it is nonsensical to include it here
        const size_t nanosecsSize = sizeof(uint64_t);
        size_t bytesToSend {0};
        size_t sent {0};

        if (autogenerateIPHeader()) {
            uint8_t packet[headerSize + nanosecsSize];
            bytesToSend = constructICMPPacket(packet, headerSize, nanosecsSize, seqNum, processIDNum);
            sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
        } else {
            size_t ipHeaderSize = m_ipHeader.getHeaderLenghtInBytes();
            // create memory buffer to load the packet unto
            uint8_t packet[ipHeaderSize + headerSize + nanosecsSize];
            bytesToSend = constructICMPPacketWithIPHeader(packet, headerSize, nanosecsSize, seqNum, processIDNum);
            sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));
        }

        if (sent == -1) {
            throw RawSocketException("Ping to " + destIP + " failed. \n Reason: \n" + std::system_category().message(errno));
        } else if (bytesToSend != sent) {
            throw RawSocketException("Not all of the packet was sent.\n Packet size: " + std::to_string(bytesToSend) + "\nSent: " + std::to_string(sent));
        }
        return sent;
    }

    void receivePing(uint8_t packet[IP_MAXPACKET], const std::string& originIP = "", const Int& seqNum = 0,  const Int& processIDNum = 0) const {
        sockaddr_storage origin {};
        socklen_t originAddrLen = sizeof(origin);

        bool acceptPacket = false;

        while (!acceptPacket) {
            int64_t numBytesRecv = recvfrom(m_socket, packet, IP_MAXPACKET, 0, reinterpret_cast<sockaddr*>(&origin), &originAddrLen);
            if (numBytesRecv == -1) {
                throw RawSocketException("The socket was not able to read the incoming ping: \n Reason: \n " + std::system_category().message(errno));
            }

            uint8_t ipHeaderReceiveBuffer[sizeof(ip)] {};
            std::memcpy(ipHeaderReceiveBuffer, packet, sizeof(ip));
            IPv4Header ipHeaderReceive {IPv4Header(ipHeaderReceiveBuffer)};
            icmphdr ICMPHeaderReceive {};
            std::memcpy(&ICMPHeaderReceive,packet + sizeof(ip), sizeof(icmphdr));


            if (!originIP.empty() && m_ipVersion == AF_INET) {

                char recvIPBuffer[INET_ADDRSTRLEN] {};
                const char *convResult = inet_ntop(m_ipVersion,
                                                   &(reinterpret_cast<sockaddr_in *>(&origin)->sin_addr),
                                                   recvIPBuffer, INET_ADDRSTRLEN);
                if (convResult == nullptr) {
                    throw RawSocketException("Conversion of the received packet's IP address was unsuccessful. \n Reason: \n" + std::system_category().message(errno));
                }
                acceptPacket = strcmp(originIP.c_str(), recvIPBuffer) == 0;

            } else if (!originIP.empty() && m_ipVersion == AF_INET6) {

                char recvIPBuffer[INET6_ADDRSTRLEN] {};
                const char *convResult = inet_ntop(m_ipVersion, &(reinterpret_cast<sockaddr_in6*>(&origin)->sin6_addr), recvIPBuffer, INET6_ADDRSTRLEN);

                if (convResult == nullptr) {
                    throw RawSocketException("Conversion of the received packet's IP address was unsuccessful. \n Reason: \n" + std::system_category().message(errno));
                }
                acceptPacket = strcmp(originIP.c_str(), recvIPBuffer) == 0;
            }
            // Anti Tampering and Packet Integrity Checks
            acceptPacket = (originIP.empty() || acceptPacket) && ipHeaderReceive.getProtocol() == IPPROTO_ICMP
            && (seqNum == 0 || seqNum == ntohs(ICMPHeaderReceive.un.echo.sequence))
            && (processIDNum == 0  || processIDNum == ntohs(ICMPHeaderReceive.un.echo.id))
            && ICMPHeaderReceive.type == ICMP_ECHOREPLY;
        }
    }

    static size_t constructARPEchoRequestPacket(uint8_t* packet, const uint32_t& dstIPAddress,
        const uint32_t& srcIPAddress, const std::array<uint8_t,6>& dstMacAddress, const std::array<uint8_t,6>& srcMacAddress) {

        ether_header ethernetHeader {};
        std::memcpy(ethernetHeader.ether_dhost, dstMacAddress.data(), 6);
        std::memcpy(ethernetHeader.ether_shost, srcMacAddress.data(), 6);
        ethernetHeader.ether_type = htons(ETHERTYPE_ARP);


        IPv4ToMACARPPacket arpPacket {};
        arpPacket.arpOperationCode = htons(ARPOP_REQUEST); // type of ARP Packet
        std::memcpy(arpPacket.srcMAC, srcMacAddress.data(), 6);
        arpPacket.srcIPv4 = srcIPAddress;
        arpPacket.dstIPv4 = dstIPAddress;

        //populate the packet
        std::memcpy(packet, &ethernetHeader, sizeof(ether_header));
        std::memcpy(packet + sizeof(ether_header), &arpPacket, sizeof(arpPacket));

        return sizeof(ether_header) + sizeof(IPv4ToMACARPPacket);
    }

    size_t sendArpEchoRequest(const std::string& dstIPAddress, const std::string& srcMacAddress, const std::string& srcIPAddress, const std::string& srcInterfaceName) {
        if (m_ipVersion != AF_PACKET) {
            throw RawSocketException("This socket needs the ipVersion == AF_PACKET to handle layer two traffic");
        }
        if (dstIPAddress.size() > 15 || dstIPAddress.size() < 7) {
            throw RawSocketException("The Destination IP address " + dstIPAddress + " is not in an IPv4 format");
        }
        uint32_t dstIPAddressNum {};
        uint32_t srcIPAddressNum {};
        Int conversionResult = inet_pton(AF_INET, dstIPAddress.c_str(), &dstIPAddressNum);
        if (conversionResult != 1) {
            throw RawSocketException("It was not possible to convert the destination address " + dstIPAddress + " into its binary form \nReason:\n " + std::system_category().message(errno));
        }
        conversionResult = inet_pton(AF_INET, srcIPAddress.c_str(), &srcIPAddressNum);
        if (conversionResult != 1) {
            throw RawSocketException("It was not possible to convert the source address " + srcIPAddress + " into its binary form \nReason:\n " + std::system_category().message(errno));
        }

        std::array<uint8_t,6> dstMACAddrBytes {};
        dstMACAddrBytes.fill(std::numeric_limits<uint8_t>::max());
        std::array<uint8_t,6> srcMACAddrBytes = Tools::stringToMac(srcMacAddress);

        sockaddr_ll destination {};
        destination.sll_family = AF_PACKET;
        destination.sll_protocol = htons(ETH_P_ARP);
        const uint32_t interfaceIndex = if_nametoindex(srcInterfaceName.c_str());
        if (interfaceIndex == 0) {
            throw RawSocketException("The interface name \"" + srcInterfaceName + "\" could not be found\nReason:\n" + std::system_category().message(errno));
        }
        destination.sll_ifindex = interfaceIndex;
        std::memcpy(destination.sll_addr, dstMACAddrBytes.data(), sizeof(dstMACAddrBytes));
        destination.sll_halen = sizeof(dstMACAddrBytes);


        uint8_t packet[sizeof(ether_header) + sizeof(IPv4ToMACARPPacket)];
        size_t bytesToSend = constructARPEchoRequestPacket(packet, dstIPAddressNum, srcIPAddressNum, dstMACAddrBytes, srcMACAddrBytes);
        size_t sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));

        if (sent == -1) {
            throw RawSocketException("ARP Echo Request to " + dstIPAddress + " failed. \n Reason: \n" + std::system_category().message(errno));
        } else if (sent != bytesToSend) {
            throw RawSocketException("Not all of the packet was sent.\n Packet size: " + std::to_string(bytesToSend) + "\nSent: " + std::to_string(sent));
        }
        return sent;
    }

    void receiveArpEchoReply(uint8_t recvBuffer[ETH_FRAME_LEN], const std::string originIP = "") {
        if (m_ipVersion != AF_PACKET) {
           throw RawSocketException("This socket needs the ipVersion == AF_PACKET to handle layer two traffic");
        }
        sockaddr_ll origin {};
        socklen_t originAddrLen = sizeof(origin);

        bool acceptPacket = false;

        while (!acceptPacket) {
            int64_t numBytesRecv = recvfrom(m_socket, recvBuffer, ETH_FRAME_LEN, 0, reinterpret_cast<sockaddr*>(&origin), &originAddrLen);
            if (numBytesRecv == -1) {
                throw RawSocketException("The socket was not able to read the incoming arp echo reply: \n Reason: \n " + std::system_category().message(errno));
            }

            ether_header ethernetHeaderResponse {};
            std::memcpy(&ethernetHeaderResponse, recvBuffer, sizeof(ether_header));

            IPv4ToMACARPPacket arpResponse {};
            std::memcpy(&arpResponse, recvBuffer + sizeof(ether_header), sizeof(IPv4ToMACARPPacket));
            char srcIPAddress[INET_ADDRSTRLEN] {};
            const char* convResult = inet_ntop(AF_INET, &arpResponse.srcIPv4, srcIPAddress,INET_ADDRSTRLEN);
            if (convResult == nullptr) {
                throw RawSocketException("Conversion of the received packet's IP address was unsuccessful. \n Reason: \n" + std::system_category().message(errno));
            }

            // Anti Tampering and Packet Integrity Checks
            acceptPacket = (originIP.empty() || (!originIP.empty() && strcmp(originIP.c_str(), srcIPAddress) == 0))
            && std::equal(std::begin(ethernetHeaderResponse.ether_shost), std::end(ethernetHeaderResponse.ether_shost), std::begin(arpResponse.srcMAC))
            && ntohs(ethernetHeaderResponse.ether_type) == ETHERTYPE_ARP
            && ntohs(arpResponse.arpOperationCode) == ARPOP_REPLY
            && arpResponse.hardAddrLength == ETH_ALEN && arpResponse.protoAddrLength == 4
            && (ntohs(arpResponse.hardAddrFormat) == ARPHRD_ETHER) && (ntohs(arpResponse.protoAddrFormat) == ETH_P_IP)
            && origin.sll_pkttype == PACKET_HOST;
        }
    }
};


#endif //SOCKS_RAWSOCKET_H