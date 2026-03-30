

#ifndef SOCKS_SOCKET_H
#define SOCKS_SOCKET_H
#include "types.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/sockios.h>
#include <unistd.h>

#include "Exceptions.h"

class  Socket {
public:
    Int m_socket {-1};

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


    ~Socket() {
        close(m_socket);
    }
};

#endif //SOCKS_SOCKET_H