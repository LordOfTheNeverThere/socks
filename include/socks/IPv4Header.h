//
// Created by miguel on 3/5/26.
//

#ifndef SOCKS_IPHEADER_H
#define SOCKS_IPHEADER_H

#include <cstring>
#include <string>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "types.h"

class IPv4Header {
public:
    ip* header;

    IPv4Header() : header(nullptr) {}

    IPv4Header(void* buffer) {
        header = reinterpret_cast<struct ip*>(buffer);
    }

    IPv4Header(void* buffer, const char* srcIP, const char* destIP, uint16_t packetSize, Int protocol) {
        header = reinterpret_cast<struct ip*>(buffer);
        std::memset(header, 0, sizeof(struct ip));

        setVersion(4);
        setIHL(5);              // Standard 20-byte header
        setTotalLength(packetSize);
        setTTL(64);             // Default TTL
        setProtocol(protocol);
        setSource(srcIP);
        setDestination(destIP);
    }

    uint8_t  getVersion()      const { return header->ip_v; }
    uint8_t  getIHL()          const { return header->ip_hl; }
    uint8_t  getTOS()          const { return header->ip_tos; }
    uint16_t getTotalLength()  const { return ntohs(header->ip_len); }
    uint16_t getID()           const { return ntohs(header->ip_id); }
    uint8_t  getTTL()          const { return header->ip_ttl; }
    uint8_t  getProtocol()     const { return header->ip_p; }
    uint16_t getChecksum()     const { return ntohs(header->ip_sum); }

    std::string getSourceStr() const {
        return inet_ntoa(header->ip_src);
    }
    std::string getDestStr() const {
        return inet_ntoa(header->ip_dst);
    }

    void setVersion(uint8_t v)      { header->ip_v = v; }
    void setIHL(uint8_t hl)          { header->ip_hl = hl; }
    void setTOS(uint8_t tos)         { header->ip_tos = tos; }
    void setTotalLength(uint16_t l)  { header->ip_len = htons(l); }
    void setID(uint16_t id)          { header->ip_id = htons(id); }
    void setTTL(uint8_t ttl)         { header->ip_ttl = ttl; }
    void setProtocol(uint8_t p)      { header->ip_p = p; }
    void setChecksum(uint16_t c)     { header->ip_sum = htons(c); }

    void setSource(const char* ip) {
        inet_pton(AF_INET, ip, &(header->ip_src));
    }

    void setDestination(const char* ip) {
        inet_pton(AF_INET, ip, &(header->ip_dst));
    }
};


#endif //SOCKS_IPHEADER_H