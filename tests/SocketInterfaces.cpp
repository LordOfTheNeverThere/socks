#include "socks/SocketInterfaces.h"

#include <gtest/gtest.h>


TEST(MethodChecking, LocalhostResponse) {
    SocketInterfaces socket;
    socket.getLocalInterfaces("8080", 0, 0);

    for (auto interface: socket.getInterfaces()) {

    }

}


TEST(MethodChecking, RemoteHostResponse) {

}
