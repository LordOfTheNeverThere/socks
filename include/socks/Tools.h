//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_TOOLS_H
#define SOCKS_TOOLS_H
#include <iomanip>
#include <iostream>
#include <sstream>
#include <fstream>
#include <array>
#include <vector>
#include <arpa/inet.h>
#include <bits/socket.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#include <netpacket/packet.h>
#include "Exceptions.h"
#include "types.h"

class Tools {
    public:

    Tools() {};

    uint16_t static add16BitOnesComplement(const uint8_t* packetPtr, const uint64_t& packetSize) {
        uint32_t bucketSum = 0;
        for (int i = 0; i < packetSize; i = i + 2) {
            uint16_t twoBytes {0};
            if (i == packetSize -1) { //handling oddbytes
                twoBytes = (packetPtr[i] << 8);
            } else {
                twoBytes = (packetPtr[i] << 8) + packetPtr[i + 1];
            }
            bucketSum = bucketSum + twoBytes;
        }

        while (bucketSum > 0xFFFF) {
            const uint16_t carryOver = (bucketSum >> 16);
            bucketSum = bucketSum & 0xFFFF;
            bucketSum = bucketSum + carryOver;
        }

        return static_cast<uint16_t>(bucketSum);
    }

    static std::string macToString(uint8_t macInBytes[ETH_ALEN]) {
        std::ostringstream oss {};

        for (int i = 0; i < ETH_ALEN; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(macInBytes[i]);
        }
        return  oss.str();
    }

    static std::string macToString(struct sockaddr_ll *s) {
        std::ostringstream oss;

        for (int i = 0; i < s->sll_halen; ++i) {
            // Make the stream interpret the following values as hexadecimal, each input will be at least two digits wide, and if there is a zero it should be printed as such and not as space
            oss << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(s->sll_addr[i]);
        }

        return oss.str();
    }

    static uint8_t hexToUInt8(const char& hexDigit) {
        if (hexDigit >= '0' && hexDigit <= '9') {
            return hexDigit - '0';
        } else if (hexDigit >= 'a' && hexDigit <= 'f') {
            return hexDigit - 'a' + 10;
        } else if (hexDigit >= 'A' && hexDigit <= 'F') {
            return hexDigit - 'A' + 10;
        } else {
            std::string hexDigitInString {hexDigit};
            throw WrongHexDigitFormat(hexDigitInString);
        }
    }

    static uint8_t fromHexByteStringToUInt8(const std::string& hexByte) {
        if (hexByte.size() != 2) {
            throw WrongHexByteFormat(hexByte);
        }
        return (hexToUInt8(hexByte[0]) << 4) + hexToUInt8(hexByte[1]);
    }

    static std::array<uint8_t, 6> stringToMac(const std::string& macAddress) {
        if (macAddress.size() != 12) {
            throw WrongMACFormat(macAddress);
        }
        std::array<uint8_t, 6> result {};
        for (Int i = 0; i < macAddress.size(); i = i + 2) {
            result[i/2] = fromHexByteStringToUInt8(macAddress.substr(i, 2));
        }
        return result;
    }


    static std::string getDefaultGateway() { // Only For Linux and IPv4
        std::ifstream routeFile("/proc/net/route");
        if (!routeFile.is_open()) {
            return "Error: Could not open /proc/net/route";
        }

        std::string line {};
        // Skip the header line
        std::getline(routeFile, line);

        while (std::getline(routeFile, line)) {
            std::stringstream ss(line);
            std::string iface {};
            std::string dest {};
            std::string gateway {};
            ss >> iface >> dest >> gateway;

            if (dest == "00000000") {
                uint32_t addr {};
                std::stringstream converter {};
                converter << std::hex << gateway;
                converter >> addr;

                in_addr in {};
                in.s_addr = addr;

                char ipStr[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &in, ipStr, INET_ADDRSTRLEN)) {
                    return std::string(ipStr);
                }
            }
        }

        return "No default gateway found";
    }

    static bool belongsToSubnetIPv4(const uint32_t ipQuery, const uint32_t ip, const uint32_t mask) { // Endiness Agnostic
        return (ipQuery & mask)==(ip & mask);
    }

    static bool belongsToSubnetIPv6(const in6_addr& ipQuery, const in6_addr& ip, const in6_addr& mask) { // Endiness Agnostic
        for (int i = 0; i < 16; ++i) {
            if ((ipQuery.s6_addr[i] & mask.s6_addr[i]) != (ip.s6_addr[i] & mask.s6_addr[i])) {
                return false;
            }
        }
        return true;
    }

    static uint32_t getNumberOfIPsInMaskIPv4(const uint32_t maskInBits) {return ~(ntohl(maskInBits)) + 1;} // Little Endian Only


    static std::vector<uint32_t> generateIPv4RangeHostEndiness( uint32_t ipInBits,  uint32_t maskInBits) { // Little Endian Only

        uint32_t numberOfIPs {Tools::getNumberOfIPsInMaskIPv4(maskInBits)};
        std::vector<uint32_t> resultVector {};
        resultVector.reserve(numberOfIPs);
        // For calculations
        ipInBits = ntohl(ipInBits);
        maskInBits = ntohl(maskInBits);

        for (uint32_t i = 0; i < numberOfIPs; ++i) {
            resultVector.push_back(htonl((ipInBits & maskInBits) | i));
        }
        return resultVector;
    }





    static void getOutputFromCommand(std::string& output, const std::string& cmd, bool& continueRunning) {

        FILE* pipe = popen(cmd.c_str(), "r");

        if (!pipe) {
            output = "Failed to run command.";
            pclose(pipe);
            return;
        }
        try {

            char buffer[5000] {};
            while (continueRunning) {
                while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                    output += buffer;
                    //std::cout << "Captured the output " << buffer << '\n';
                }
            }
        } catch (...) {
            pclose(pipe);
        }
        pclose(pipe);
    }


};

#endif //SOCKS_TOOLS_H