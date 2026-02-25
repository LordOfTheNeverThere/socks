//
// Created by miguel on 2/25/26.
//

#ifndef SOCKS_CLIENTSOCKETEXCEPTION_H
#define SOCKS_CLIENTSOCKETEXCEPTION_H
#include "GenericException.h"


class ClientSocketException : public GenericException {
public:
    ClientSocketException(const std::string& error) : GenericException(error) {}
};
#endif //SOCKS_CLIENTSOCKETEXCEPTION_H