// As implemented in <net/if_arp.h>
#ifndef SOCKS_IPV4TOMACARPHEADER_H
#define SOCKS_IPV4TOMACARPHEADER_H
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in.h>

#define ARPFIXEDHEADERSIZE 8; // Size of the header that is fixed regardless of layer 3 or 2 addressing
struct __attribute__((packed)) IPv4ToMACARPPacket { // Contains the Fixed Header plus the payload for IPv4 to MAC

    uint16_t hardAddrFormat = htons(ARPHRD_ETHER);;		// Format of hardware address.  (Default ARPHRD_ETHER)
    uint16_t protoAddrFormat = htons(ETH_P_IP);		// Format of protocol address.  (Default ETH_P_IP)
    uint8_t hardAddrLength = ETH_ALEN;		// Length of hardware address.  (Default ETH_ALEN)
    uint8_t protoAddrLength = 4;		// Length of protocol address.  (Default 4)
    uint16_t arpOperationCode;		                // ARP opcode (command).  */

    uint8_t srcMAC[ETH_ALEN];      // Sender Hardware Address (MAC)
    uint32_t srcIPv4;              // Sender Protocol Address (IPv4)
    uint8_t dstMAC[ETH_ALEN];      // Target Hardware Address (MAC) (Ignored in ARP Echo Request)
    uint32_t dstIPv4;              // Target Protocol Address (IPv4)
};

static_assert(sizeof(IPv4ToMACARPPacket) == 28, "Padding detected in IPv4ARPHeader!");
#endif //SOCKS_IPV4TOMACARPHEADER_H