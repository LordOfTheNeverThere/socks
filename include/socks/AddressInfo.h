
#ifndef SOCKS_IPADDRESS4_H
#define SOCKS_IPADDRESS4_H
#include <string>
#include <sys/socket.h>


struct AddressInfo {
    int family;
    int socketType;
    int protocol;
    socklen_t ipLength;
    sockaddr_storage addr;
    std::string ipString;
    int port;
};


#endif //SOCKS_IPADDRESS4_H