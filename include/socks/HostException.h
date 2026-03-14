//
// Created by miguel on 3/14/26.
//

#ifndef SOCKS_HOSTEXCEPTION_H
#define SOCKS_HOSTEXCEPTION_H
#include "GenericException.h"

class HostException : public GenericException {
public:
    HostException(const std::string& error) : GenericException(error) {}
};

#endif //SOCKS_HOSTEXCEPTION_H