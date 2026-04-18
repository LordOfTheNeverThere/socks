

#ifndef SOCKS_SOCKET_H
#define SOCKS_SOCKET_H
#include "types.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <unistd.h>
#include <utility>

#include "Exceptions.h"

class  Socket {
public:
    Int m_socket {-1};
    Socket(Int newSocket): m_socket(newSocket){};
    Socket(){};

    void setSocketAsNonBlock() const {
        Int result {fcntl(m_socket, F_SETFL, O_NONBLOCK)};
        if (result == -1) {
            throw FileDescriptorOptionException(O_NONBLOCK);
        }
        //By setting a socket to non-blocking, you can effectively “poll” the socket for information.
        //If you try to read from a non-blocking socket and there’s no data there, it’s not allowed to block—it will return -1
        //and errno will be set to EAGAIN or EWOULDBLOCK.
    }

    void setSocketAsBlock() const {
        int flags = fcntl(m_socket, F_GETFL, 0);
        if (flags == -1) {
            throw FileDescriptorOptionException(0);
        }

        flags &= ~O_NONBLOCK; // clear non block flag
        int result = fcntl(m_socket, F_SETFL, flags);

        if (result == -1) {
            throw FileDescriptorOptionException(flags);
        }
    }

    void setSocketReceiveBuffer(const uint64_t bufferSize) {
        socklen_t varSize {sizeof(bufferSize)};
        Int result {setsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &bufferSize, varSize)};
        if (result == -1) {
            throw FileDescriptorOptionException(SO_RCVBUF);
        }

        uint64_t bufferSizeEffective {};
        getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &bufferSizeEffective, &varSize);

        if (bufferSize != bufferSizeEffective) {
            // The OS capped our request, let's do it overriding the OS capping
            result = setsockopt(m_socket, SOL_SOCKET, SO_RCVBUFFORCE, &bufferSize, varSize);
            if (result == -1) {
                throw FileDescriptorOptionException(SO_RCVBUFFORCE);
            }
        }
    }

    void setSocketSendBuffer(const uint64_t bufferSize) {
        socklen_t varSize {sizeof(bufferSize)};
        Int result {setsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &bufferSize, varSize)};
        if (result == -1) {
            throw FileDescriptorOptionException(SO_SNDBUF);
        }

        uint64_t bufferSizeEffective {};
        getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &bufferSizeEffective, &varSize);

        if (bufferSize != bufferSizeEffective) {
            // The OS capped our request, let's do it overriding the OS capping
            result = setsockopt(m_socket, SOL_SOCKET, SO_SNDBUFFORCE, &bufferSize, varSize);
            if (result == -1) {
                throw FileDescriptorOptionException(SO_SNDBUFFORCE);
            }
        }
    }
    void setSocketOpt(const Int level ,const Int opt, const Int optVal) {
        if (setsockopt(m_socket, level, opt, &optVal, sizeof(optVal)) == -1) {
            throw  FileDescriptorOptionException(opt);
        }
    }

    void setSocketIPHeaderManually(const Int val) {

        Int socketOpt = setsockopt(m_socket,IPPROTO_IP,IP_HDRINCL, &val, sizeof(val));
        if (socketOpt == -1) {
            throw SocketOptionException(IP_HDRINCL);
        }
    }

    uint64_t getSocketSndBuffer() const {
        uint64_t bufferSize{};
        socklen_t bufferVarSize{sizeof(bufferSize)};
        getsockopt(m_socket, SOL_SOCKET, SO_SNDBUF, &bufferSize, &bufferVarSize);
        return bufferSize;
    }

    uint64_t getSocketRcvBuffer() const {
        uint64_t bufferSize{};
        socklen_t bufferVarSize{sizeof(bufferSize)};
        getsockopt(m_socket, SOL_SOCKET, SO_RCVBUF, &bufferSize, &bufferVarSize);
        return bufferSize;
    }


    uint64_t getSocketOverflowDrops() const {
        uint64_t drops{};
        socklen_t dropsVarSize{sizeof(drops)};
        getsockopt(m_socket, SOL_SOCKET, SO_RXQ_OVFL, &drops, &dropsVarSize);
        return drops;
    }

    void setSocketSendTimeoutValue(const timeval& timeout) const {
        if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == -1) {
            throw  FileDescriptorOptionException(SO_SNDTIMEO);
        }
    }


    void closeSocket() {
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1; // Prevent double-close or closing wrong FDs
        }
    }

    // copy constructor deleted
    Socket(const Socket& other) = delete;

    // copy assignment operator deleted
    Socket& operator=(const Socket& other) = delete;

    // move constructor
    Socket(Socket&& other) noexcept
        : m_socket(std::exchange(other.m_socket, -1)) {
    }

    //  move assignment operator
    Socket& operator=(Socket&& other) noexcept {
        if (this != &other) {
            closeSocket(); // Clean up our current resource first
            // Transfer ownership and reset the source
            m_socket = std::exchange(other.m_socket, -1);
        }
        return *this;
    }
    ~Socket() {
        closeSocket();
    }
};

#endif //SOCKS_SOCKET_H