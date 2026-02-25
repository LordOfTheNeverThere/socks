//
// Created by miguel on 2/25/26.
//

#ifndef SOCKS_NAMERESOLUTIONEXCEPTION_H
#define SOCKS_NAMERESOLUTIONEXCEPTION_H
#include "GenericException.h"

class NameResolutionException : public GenericException {
public:
    NameResolutionException(const std::string& error) : GenericException(error) {}
};



#endif //SOCKS_NAMERESOLUTIONEXCEPTION_H