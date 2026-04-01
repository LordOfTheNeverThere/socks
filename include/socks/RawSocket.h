
#ifndef SOCKS_RAWSOCKET_H
#define SOCKS_RAWSOCKET_H
#include <array>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <net/if.h>
#include "Socket.h"
#include <netinet/ip_icmp.h>
#include <net/ethernet.h>
#include <net/if_arp.h>

#include "IPv4Header.h"
#include "Tools.h"
#include "IPv4ToMACARPPacket.h"
#include "LocalHost.h"


class RawSocket : public Socket {

private:
    Int m_ipVersion {0};
    Int m_protocol {0};
public:
    RawSocket(){};
    RawSocket(const Int ipVersion, const Int protocol) :  m_protocol(protocol) {

        if (ipVersion != AF_INET && ipVersion != AF_INET6 && ipVersion != AF_PACKET) {
            throw UnsupportedAFTypeException(ipVersion);
        } else {
            m_ipVersion = ipVersion;
        }

        m_socket = socket(ipVersion, SOCK_RAW, protocol);
        if (m_socket == -1) {
            throw SocketCreationException();
        }
    }

    size_t constructICMPPacketWithIPHeader(uint8_t* packet, const size_t headerSize, const size_t nanosecsSize, const uint16_t seqNum, const uint16_t processIDNum, IPv4Header ipHeader)  {

        ipHeader.setProtocol(IPPROTO_ICMP);
        size_t ipHeaderSize {ipHeader.getHeaderLenghtInBytes()};
        ipHeader.setTotalLength(ipHeaderSize + headerSize + nanosecsSize);
        std::memset(packet, 0, ipHeaderSize + headerSize + nanosecsSize);
        std::memcpy(packet, ipHeader.header, ipHeaderSize);

        return ipHeaderSize + constructICMPPacket(packet + ipHeaderSize, headerSize, nanosecsSize, seqNum, processIDNum);
    }

    static size_t constructICMPPacket(uint8_t* packet, const size_t headerSize, const size_t nanosecsSize, const uint16_t seqNum, const uint16_t processIDNum) {
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

    size_t sendPingIPv4Only(const uint32_t ipNum, const uint16_t seqNum = 0, const uint16_t processIDNum = 0, IPv4Header ipHeader = IPv4Header()) {
        sockaddr_storage destination {};
        sockaddr_in *ipv4View = reinterpret_cast<sockaddr_in *>(&destination);
        ipv4View->sin_addr.s_addr = ipNum;
        ipv4View->sin_family = m_ipVersion;

        return sendPing(destination, seqNum, processIDNum, ipHeader);
    }

    size_t sendPing(sockaddr_storage& destination, const uint16_t seqNum = 0, const uint16_t processIDNum = 0, IPv4Header ipHeader = IPv4Header()) {

        const size_t headerSize = sizeof(icmphdr); //use icmphdr instead of icmp for requests, icmp has part of the IP header which caused the host to reply with an error, since this is request it is nonsensical to include it here
        const size_t nanosecsSize = sizeof(uint64_t);
        size_t bytesToSend {0};
        size_t sent {0};

        if (ipHeader.header == nullptr) {
            uint8_t packet[headerSize + nanosecsSize];
            bytesToSend = constructICMPPacket(packet, headerSize, nanosecsSize, seqNum, processIDNum);
            sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(sockaddr_in));
        } else {
            setSocketIPHeaderManually(1);
            size_t ipHeaderSize = ipHeader.getHeaderLenghtInBytes();
            // create memory buffer to load the packet unto
            uint8_t packet[ipHeaderSize + headerSize + nanosecsSize];
            bytesToSend = constructICMPPacketWithIPHeader(packet, headerSize, nanosecsSize, seqNum, processIDNum, ipHeader);
            if (m_ipVersion == AF_INET) {
                sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(sockaddr_in));
            } else if (m_ipVersion == AF_INET6) {
                sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(sockaddr_in6));
            }
        }

        if (sent == -1) {
            throw SendingException();
        } else if (bytesToSend != sent) {
            throw SendingException(bytesToSend, sent);
        }

        return sent;
    }

    size_t sendPing(const std::string& destIP, const uint16_t seqNum = 0, const uint16_t processIDNum = 0,  IPv4Header ipHeader = IPv4Header()){
        
        sockaddr_storage destination {};
        destination.ss_family = m_ipVersion;

        Int conversionResult {};
        if (m_ipVersion == AF_INET) {
            conversionResult = inet_pton(m_ipVersion, destIP.c_str(), &(reinterpret_cast<sockaddr_in*>(&destination)->sin_addr));
        } else if (m_ipVersion == AF_INET6) {
            conversionResult = inet_pton(m_ipVersion, destIP.c_str(), &(reinterpret_cast<sockaddr_in6*>(&destination)->sin6_addr));
        }
        if (conversionResult != 1) {
            throw ConversionToIPBinaryException(destIP);
        }

       return sendPing(destination, seqNum, processIDNum, ipHeader);
    }

    int64_t receivePing(uint8_t packet[IP_MAXPACKET], const std::string& originIP = "", const Int seqNum = 0,  const Int processIDNum = 0, const Int icmpType = ICMP_ECHOREPLY) const {
        sockaddr_storage origin {};
        socklen_t originAddrLen = sizeof(origin);

        bool acceptPacket = false;
        int64_t numBytesRecv{};
        sockaddr_storage trueOrigin{};

        if (!originIP.empty()) {
            Int conversionResult {};
            if (m_ipVersion == AF_INET) {
                conversionResult = inet_pton(m_ipVersion, originIP.c_str(), &(reinterpret_cast<sockaddr_in*>(&trueOrigin)->sin_addr));
            } else if (m_ipVersion == AF_INET6) {
                conversionResult = inet_pton(m_ipVersion, originIP.c_str(), &(reinterpret_cast<sockaddr_in6*>(&trueOrigin)->sin6_addr));
            }
            if (conversionResult != 1) {
                throw ConversionToIPBinaryException(originIP);
            }
        }
        while (!acceptPacket) {
            numBytesRecv = recvfrom(m_socket, packet, IP_MAXPACKET, 0, reinterpret_cast<sockaddr*>(&origin), &originAddrLen);
            if (numBytesRecv == -1) {
                throw ReceivingException();
            }

            uint8_t ipHeaderReceiveBuffer[sizeof(ip)] {};
            std::memcpy(ipHeaderReceiveBuffer, packet, sizeof(ip));
            IPv4Header ipHeaderReceive {IPv4Header(ipHeaderReceiveBuffer)};
            icmphdr ICMPHeaderReceive {};
            std::memcpy(&ICMPHeaderReceive,packet + sizeof(ip), sizeof(icmphdr));


            if (!originIP.empty() && m_ipVersion == AF_INET) {
                acceptPacket = reinterpret_cast<sockaddr_in *>(&origin)->sin_addr.s_addr == reinterpret_cast<sockaddr_in *>(&trueOrigin)->sin_addr.s_addr;

            } else if (!originIP.empty() && m_ipVersion == AF_INET6) {

                acceptPacket = std::equal( std::begin(reinterpret_cast<sockaddr_in6 *>(&origin)->sin6_addr.s6_addr),
                    std::end(reinterpret_cast<sockaddr_in6 *>(&origin)->sin6_addr.s6_addr),
                    std::begin(reinterpret_cast<sockaddr_in6 *>(&trueOrigin)->sin6_addr.s6_addr));
            }
            // Anti Tampering and Packet Integrity Checks
            acceptPacket = (originIP.empty() || acceptPacket) && ipHeaderReceive.getProtocol() == IPPROTO_ICMP
            && (seqNum == 0 || seqNum == ntohs(ICMPHeaderReceive.un.echo.sequence))
            && (processIDNum == 0  || processIDNum == ntohs(ICMPHeaderReceive.un.echo.id))
            && (ICMPHeaderReceive.type == ICMP_ECHOREPLY || ICMPHeaderReceive.type == icmpType); // We always catch at least Echo replies, however other types can also be caught
        }
        return numBytesRecv;
    }


    Int pingCheck(const std::string& destination, const Int numOfPings) {

        setSocketAsNonBlock();

        auto sendingFunc = [&] {
            for (int i = 0; i < numOfPings; ++i) {
                sendPing(destination);
            }
        };

        auto receivingFunc = [&] () -> Int {
            Int numReceived {};
            Int epollFD = epoll_create1(0);
            if (epollFD == -1) {
                throw EpollCreationException();
            }
            try {
                epoll_event ev {};
                ev.events = EPOLLIN;
                ev.data.fd = m_socket;
                Int result = epoll_ctl(epollFD, EPOLL_CTL_ADD, m_socket, &ev);
                if (result == -1) {
                    throw EpollControllerException(EPOLL_CTL_ADD);
                }

                while (true) {
                    result = epoll_wait(epollFD, &ev, 1,10000);
                    if (result == -1) {
                        throw EpollWaitException();
                    } else if (result == 0) {
                        //timeout
                        close(epollFD);
                        return numReceived;
                    }

                    if (ev.data.fd == m_socket && ev.events & EPOLLIN) {
                        std::array<uint8_t, IP_MAXPACKET> recvBuffer {};
                        int64_t numBytesRecv {};
                        try {
                            numBytesRecv = receivePing(recvBuffer.data(), destination);
                        } catch (std::exception& e) {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                continue; //EWOULDBLOCK or EAGAIN, kernel dropped the packet for some reason, such as CRC corruption
                            } else {
                                std::cerr << e.what() << '\n';
                            }
                        }
                        if (numBytesRecv > 0) {
                            numReceived++;
                        }
                    }
                }
            } catch (std::runtime_error& e) {
                close(epollFD);
                std::cerr << e.what() << '\n';
                return numReceived;
            }
        };


        std::thread sender(sendingFunc);
        std::future<Int> receiver(std::async(receivingFunc));

        sender.join();
        setSocketAsBlock();
        return receiver.get();
    }



    static size_t constructARPEchoRequestPacket(uint8_t* packet, const uint32_t dstIPAddress,
        const uint32_t srcIPAddress, const std::array<uint8_t,6>& dstMacAddress, const std::array<uint8_t,6>& srcMacAddress) {

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

    size_t sendArpEchoRequest(const uint32_t dstIPInBits, const std::string& srcMacAddress, const uint32_t srcIPInBits, const std::string& srcInterfaceName) const {

        std::array<uint8_t,6> srcMACAddrBytes = Tools::stringToMac(srcMacAddress);
        std::array<uint8_t,6> dstMACAddrBytes {};
        dstMACAddrBytes.fill(std::numeric_limits<uint8_t>::max());
        sockaddr_ll destination {};
        destination.sll_family = AF_PACKET;
        destination.sll_protocol = htons(ETH_P_ARP);
        const uint32_t interfaceIndex = if_nametoindex(srcInterfaceName.c_str());
        if (interfaceIndex == 0) {
            throw InterfaceNotFoundByName(srcInterfaceName);
        }
        destination.sll_ifindex = interfaceIndex;
        std::memcpy(destination.sll_addr, dstMACAddrBytes.data(), sizeof(dstMACAddrBytes));
        destination.sll_halen = sizeof(dstMACAddrBytes);

        uint8_t packet[sizeof(ether_header) + sizeof(IPv4ToMACARPPacket)];
        size_t bytesToSend = constructARPEchoRequestPacket(packet, dstIPInBits, srcIPInBits, dstMACAddrBytes, srcMACAddrBytes);
        size_t sent = sendto(m_socket, packet, bytesToSend, 0, reinterpret_cast<sockaddr*>(&destination), sizeof(destination));

        if (sent == -1) {
            throw SendingException();
        } else if (bytesToSend != sent) {
            throw SendingException(bytesToSend, sent);
        }
        return sent;
    }

    size_t sendArpEchoRequest(const std::string& dstIPAddress, const std::string& srcMacAddress, const std::string& srcIPAddress, const std::string& srcInterfaceName) {
        if (m_ipVersion != AF_PACKET) {
            throw UnsupportedAFTypeException(m_ipVersion);
        }
        if (dstIPAddress.size() > 15 || dstIPAddress.size() < 7) {
            throw WrongIPv4Format(dstIPAddress);
        }
        uint32_t dstIPAddressNum {};
        uint32_t srcIPAddressNum {};
        Int conversionResult = inet_pton(AF_INET, dstIPAddress.c_str(), &dstIPAddressNum);
        if (conversionResult != 1) {
            throw ConversionToIPBinaryException(dstIPAddress);
        }
        conversionResult = inet_pton(AF_INET, srcIPAddress.c_str(), &srcIPAddressNum);
        if (conversionResult != 1) {
            throw ConversionToIPBinaryException(srcIPAddress);
        }

        return sendArpEchoRequest(dstIPAddressNum, srcMacAddress, srcIPAddressNum, srcInterfaceName);
    }

    int64_t receiveArpEchoReply(uint8_t recvBuffer[ETH_FRAME_LEN], const std::string originIP = "") const {
        if (m_ipVersion != AF_PACKET) {
            throw UnsupportedAFTypeException(m_ipVersion);
        }
        sockaddr_ll origin {};
        socklen_t originAddrLen = sizeof(origin);

        bool acceptPacket = false;
        int64_t numBytesRecv {};
        while (!acceptPacket) {
            numBytesRecv = recvfrom(m_socket, recvBuffer, ETH_FRAME_LEN, 0, reinterpret_cast<sockaddr*>(&origin), &originAddrLen);
            if (numBytesRecv == -1 && (errno == EWOULDBLOCK | errno == EAGAIN)) {
                return 0;
            } else if (numBytesRecv == -1) {
                throw ReceivingException();
            }

            ether_header ethernetHeaderResponse {};
            std::memcpy(&ethernetHeaderResponse, recvBuffer, sizeof(ether_header));

            IPv4ToMACARPPacket arpResponse {};
            std::memcpy(&arpResponse, recvBuffer + sizeof(ether_header), sizeof(IPv4ToMACARPPacket));
            char srcIPAddress[INET_ADDRSTRLEN] {};
            const char* convResult = inet_ntop(AF_INET, &arpResponse.srcIPv4, srcIPAddress,INET_ADDRSTRLEN);
            if (convResult == nullptr) {
                throw ConversionToIPStringException();
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
        return numBytesRecv;
    }
};


#endif //SOCKS_RAWSOCKET_H