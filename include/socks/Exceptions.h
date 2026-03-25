//
// Created by miguel on 2/25/26.
//

#ifndef SOCKS_GENERICEXCEPTION_H
#define SOCKS_GENERICEXCEPTION_H
#include <netdb.h>
#include <stdexcept>
#include <system_error>
#include "types.h"


class ConfigException : public std::runtime_error {
public:
    ConfigException(const std::string& error) : std::runtime_error(error) {}
};


class SystemCallException : public std::runtime_error {
public:
    SystemCallException(const std::string& error) : std::runtime_error(error) {}
};


class NameResolutionException : public SystemCallException {
public:
    NameResolutionException(const std::string& hostname, const Int ipVersion, const std::string& port, const Int status)
    : SystemCallException("The name resolution was not able to find any viable interfaces for the socket connection to "
        + hostname + " on port: " + port + " and IP version: " + std::to_string(ipVersion) + ".\n"
        + "Prompting the error with status code: " + gai_strerror(status)) {}
};

class UnresolvedInternalInterfacesException : public SystemCallException {
public:
    UnresolvedInternalInterfacesException()
    : SystemCallException("The resolution of the machine's address, gateway, netmask and name was unsuccessful.\nReason:\n "+ std::system_category().message(errno)) {}
};

class ConversionToIPStringException : public SystemCallException {
public:
    ConversionToIPStringException()
    : SystemCallException("Conversion of the IP address to string was unsuccessful. \n Reason: \n" + std::system_category().message(errno)) {}
};

class ConversionToIPBinaryException : public SystemCallException {
public:
    ConversionToIPBinaryException(const std::string& ipString)
    : SystemCallException("Conversion of the IP " + ipString +" to binary was unsuccessful. \n Reason: \n" + std::system_category().message(errno)) {}
};

class ConnectionFailedException : public SystemCallException {
public:
    ConnectionFailedException(const std::string& ipString, const Int port)
    : SystemCallException( "The connection of the socket to IP and port: " + ipString + ":" + std::to_string(port) + " on the server failed. \n Reason: \n" + std::system_category().message(errno)) {}

    ConnectionFailedException()
    : SystemCallException("It was not possible to connect the socket to any of the interfaces, one of the reasons: \n" + std::system_category().message(errno)) {}
};

class BindingFailedException : public SystemCallException {
public:
    BindingFailedException(const Int port)
    : SystemCallException( "The binding of the socket to port: " + std::to_string(port) + " failed. \n Reason: \n" + std::system_category().message(errno)) {}

    BindingFailedException()
    :SystemCallException("It was not possible to bind the socket to any of the interfaces, one of the reasons: \n" + std::system_category().message(errno)) {}
};

class ServerListeningException : public SystemCallException {
public:
    ServerListeningException()
    : SystemCallException("Could not make a listening socket for the server. \n Reason: \n" + std::system_category().message(errno)) {}
};

class ServerAcceptingException : public SystemCallException {
public:
    ServerAcceptingException(const std::string& fromIPString)
    : SystemCallException("It was not possible to accept the connection to the socket from: " + fromIPString) {}
};

class ZombieReapingException : public SystemCallException {
public:
    ZombieReapingException()
    : SystemCallException("Could not reap zombie processes of previously closed connections") {}
};


class ServerWaitingException : public SystemCallException {
public:
    ServerWaitingException(const Int durationSeconds)
    : SystemCallException("Server waited for " + std::to_string(durationSeconds) + " seconds, but no connection attempt was made.") {}
};

class SendingException : public SystemCallException {
public:
    SendingException()
    : SystemCallException("Error sending. \n Reason: \n " + std::system_category().message(errno)) {}
    SendingException(const Int bytesToSend, const Int sent)
    : SystemCallException("Not all of the packet was sent.\n Packet size: " + std::to_string(bytesToSend) + "\nSent: " + std::to_string(sent)) {}
};

class ReceivingException : public SystemCallException {
public:
    ReceivingException()
    : SystemCallException("Error receiving. \n Reason: \n " + std::system_category().message(errno)) {}
};

class LocalHostConstructionException : public SystemCallException {
public:
    LocalHostConstructionException()
    : SystemCallException("It was not possible to populate the data") {}
};


class SocketCreationException : public SystemCallException {
public:
    SocketCreationException()
        : SystemCallException("The creation of the socket failed. \n Reason: \n" + std::system_category().message(errno)) {}
};

class SocketOptionException : public SystemCallException {
public:
    SocketOptionException(const Int option)
        : SystemCallException("The setting of the socket option " + std::to_string(option) + " failed.\nReason:\n" + std::system_category().message(errno)) {}
};

class FileDescriptorOptionException : public SystemCallException {
public:
    FileDescriptorOptionException(const Int option)
        : SystemCallException("The setting of the file descriptor option " + std::to_string(option) + " failed.\nReason:\n" + std::system_category().message(errno)) {}
};

class EpollCreationException : public SystemCallException {
public:
    EpollCreationException()
        : SystemCallException("The creation of the epoll file descriptor failed! \n Reason: \n" + std::system_category().message(errno)) {}
};

class EpollControllerException : public SystemCallException {
public:
    EpollControllerException(const Int& operation)
        : SystemCallException("The operation "+  std::to_string(operation) +" on epoll failed ! \n Reason: \n" + std::system_category().message(errno)) {}
};

class EpollWaitException : public SystemCallException {
public:
    EpollWaitException()
        : SystemCallException("There was an error while waiting for the epoll file descriptor! \n Reason: \n" + std::system_category().message(errno)) {}
};

class InterfaceNotFoundByName : public SystemCallException {
public:
    InterfaceNotFoundByName(const std::string& name)
        : SystemCallException("The interface name \"" + name + "\" could not be found\nReason:\n" + std::system_category().message(errno)) {}
};



class MissingMaskException : public ConfigException {
public:
    MissingMaskException(const std::string& mac)
        : ConfigException("The network mask of the interface with MAC: " + mac + " is missing.") {}
};

class WrongMACFormat : public ConfigException {
public:
    WrongMACFormat(const std::string& mac)
        : ConfigException("Wrong MAC format : " + mac) {}
};

class WrongHexDigitFormat : public ConfigException {
public:
    WrongHexDigitFormat(const std::string& hex)
        : ConfigException("Wrong hex digit format : " + hex) {}
};

class WrongHexByteFormat : public ConfigException {
public:
    WrongHexByteFormat(const std::string& hexByte)
        : ConfigException("Wrong hex byte format : " + hexByte) {}
};

class WrongIPv4Format : public ConfigException {
public:
    WrongIPv4Format(const std::string& ip)
        : ConfigException("Wrong IPv4 format : " + ip) {}
};

class IPv4HeaderNullBufferException : public ConfigException {
public:
    IPv4HeaderNullBufferException()
        : ConfigException("IPv4Header Constructor requires a non-null buffer.") {}
};

class UnsupportedAFTypeException : public ConfigException {
public:
    UnsupportedAFTypeException(const sa_family_t addrFamType)
        : ConfigException("The family type with ID: " + std::to_string(addrFamType) + " is not supported.") {}
};




#endif //SOCKS_GENERICEXCEPTION_H