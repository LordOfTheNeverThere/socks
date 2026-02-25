//
// Created by miguel on 2/25/26.
//

#ifndef SOCKS_SERVERSOCKETEXCEPTION_H
#define SOCKS_SERVERSOCKETEXCEPTION_H
#include "GenericException.h"

class ServerSocketException : public GenericException {
public:
    ServerSocketException(const std::string& error) : GenericException(error) {}
};


#endif //SOCKS_SERVERSOCKETEXCEPTION_H