//
// Created by miguel on 2/25/26.
//

#ifndef SOCKS_GENERICEXCEPTION_H
#define SOCKS_GENERICEXCEPTION_H
#include <stdexcept>


class GenericException : public std::runtime_error {
public:
    GenericException(const std::string& error) : std::runtime_error(error) {}
};

#endif //SOCKS_GENERICEXCEPTION_H