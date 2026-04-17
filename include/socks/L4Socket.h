
#ifndef SOCKS_SCOKET_H
#define SOCKS_SCOKET_H

#include "NetworkListener.h"
#include "Socket.h"
#include "types.h"

class  L4Socket : private Socket {
private:

    FRIEND_TEST(MethodChecking, createSocketAndBind);
    FRIEND_TEST(MethodChecking, clientAndServerMock);
    FRIEND_TEST(MethodChecking, twoClientAndServerMock);

    bool bindSocketToInterface(AddressInfo& interface) {
        bool continues = false;
        Int yes = 1;

        Int setSocketOption = setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(Int));
        // Allows usage of ports and addresses in less than 2 mins after their closure.

        if ( setSocketOption == -1) {
            exit(1);
        }


        Int binding = bind(m_socket, reinterpret_cast<sockaddr*>(&interface.addr), interface.ipLength);
        if (binding == -1) {
            closeSocket();
            continues = true;
        }
        return continues;
    }


    bool connectSocketToInterface(AddressInfo & interface) {
        bool continues = false;
        Int connecting = connect(m_socket, reinterpret_cast<sockaddr*>(&interface.addr), interface.ipLength);
        if (connecting == -1) {
            closeSocket();
            continues = true;
        }
        return continues;
    }

    void handleMultipleSends(const void *msgToSend, const size_t& msgSize, const Int& durationSeconds, sockaddr_storage& clientAddress, socklen_t& sizeClientAddress) {
        struct sigaction sa {};
        sa.sa_handler = sigChildHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
            throw ZombieReapingException();
        }
        while (true) {
            if (durationSeconds > 0) {
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(m_socket, &readfds);

                timeval tv {};
                tv.tv_sec = durationSeconds;
                tv.tv_usec = 0;
                //If durationSeconds is -1, we wait indefinitely (NULL in select)
                // The Guard: Only proceed to accept if select says data is ready on the fds mentioned in readfds
                int activity = select(m_socket + 1, &readfds, nullptr, nullptr, &tv);

                if (activity < 0 && errno == EINTR) {
                    continue; // Interrupted by signal
                }
                else if (activity == 0) {
                    //timeout reached
                    throw ServerWaitingException(durationSeconds);
                }
            }
            Int connectedSocket = accept(m_socket, reinterpret_cast<sockaddr*>(&clientAddress), &sizeClientAddress);
            if (connectedSocket == -1) {
                std::string ipNameBuffer {""};
                std::cerr << "It was not possible to accept the connection to the socket from: " ;
                if (reinterpret_cast<sockaddr*>(&clientAddress)->sa_family == AF_INET) {
                    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&clientAddress)->sin_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in));
                } else {
                    inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(&clientAddress)->sin6_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in6));
                }
                std::cerr << ipNameBuffer.data();
                continue;
            }
            if (!fork()) { // this is the child process
                closeSocket(); // child doesn't need the listener
                if (::send(connectedSocket, msgToSend, msgSize, 0) == -1)
                    std::cerr << "Error sending. \n Reason: \n " + std::system_category().message(errno);
                close(connectedSocket);
                exit(0);
            }
            close(connectedSocket);  // parent doesn't need this
        }
    }


    static void sigChildHandler(Int s){
        (void)s; // quiet unused variable warning

        // waitpid() might overwrite errno, so we save and restore it:
        const Int saved = errno;

        while(waitpid(-1, nullptr, WNOHANG) > 0) {}

        errno = saved;
    }



    void closeSocket() {
        if (m_socket != -1) {
            close(m_socket);
            m_socket = -1; // Prevent double-close or closing wrong FDs
        }
    }



public:
    L4Socket (NetworkListener& listener, const bool isServer) {

        for (AddressInfo &interface: listener.m_interfaces) {

            m_socket = socket(interface.family, interface.socketType, interface.protocol);

            if (m_socket == -1) {
                perror("Got an error while creating a socket");
                continue;
            }

            if (isServer) {
                if (bindSocketToInterface(interface)) {
                    continue;
                };
            }
            else {
                if (connectSocketToInterface(interface)) {
                    continue;
                };
            }
            return;
        }
        if (isServer) {
            throw BindingFailedException();
        }
        else {
            throw ConnectionFailedException();
        }
    }

    void makeSocketListen(const Int connectionQueueNumber = 7) {
        Int listening = listen(m_socket, connectionQueueNumber);
        if (listening == -1) {
            throw ServerListeningException();
        }
    }


    void send(const void* msgToSend, const size_t msgSize, const Int connectionQueueNumber = 7, const bool multiConnection = true, const Int durationSeconds = -1) {
        makeSocketListen(connectionQueueNumber);
        sockaddr_storage clientAddress {};
        socklen_t sizeClientAddress = sizeof(clientAddress);

        if (multiConnection) {
            handleMultipleSends(msgToSend, msgSize, durationSeconds, clientAddress, sizeClientAddress);
        } else {
            Int connectedSocket = accept(m_socket, reinterpret_cast<sockaddr*>(&clientAddress), &sizeClientAddress);
            if (connectedSocket == -1) {
                std::string ipNameBuffer {""};
                if (reinterpret_cast<sockaddr*>(&clientAddress)->sa_family == AF_INET) {
                    inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(&clientAddress)->sin_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in));
                } else {
                    inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(&clientAddress)->sin6_addr, ipNameBuffer.data(), sizeof(struct sockaddr_in6));
                }
                throw ServerAcceptingException(ipNameBuffer);
            }
            if (::send(connectedSocket, msgToSend, msgSize, 0) == -1) {
                throw SendingException();
            }
            close(connectedSocket); // message was sent no more sending required.
        }
        closeSocket(); // The Listener is no longer required after all has been sent.
    }

    Int receive(void* msgReceived, const size_t msgSize) const {
        const Int bytesReceived = static_cast<Int>(recv(m_socket, msgReceived, msgSize, 0));
        if (bytesReceived == -1) {
            throw ReceivingException();
        }
        return bytesReceived;
    }

};
#endif //SOCKS_SCOKET_H