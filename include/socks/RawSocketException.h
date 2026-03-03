//
// Created by miguel on 3/3/26.
//

#ifndef SOCKS_RAWSOCKETEXCEPTION_H
#define SOCKS_RAWSOCKETEXCEPTION_H
class RawSocketException : public GenericException {
public:
    RawSocketException(const std::string& error) : GenericException(error) {}
};

#endif //SOCKS_RAWSOCKETEXCEPTION_H