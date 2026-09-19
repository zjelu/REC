#include "Acceptor.hpp"
#include "unordered_map"
#include "Eventloop.hpp"
#include "Connection.hpp"
#include "Tcpserver.hpp"

#include <sys/socket.h>
#include <sys/eventfd.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unistd.h>


int main() {

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        std::perror("epoll_create1");
        return 1;
    }

    int event_fd = ::eventfd(
        0,
        EFD_NONBLOCK | EFD_CLOEXEC
    );

    if (event_fd == -1) {
        std::perror("eventfd");
        close(epoll_fd);
        return 1;
    }


    Poller poller(epoll_fd);
    Eventloop loop(poller);

    Tcpserver server(8080, &loop);

    server.setMessageCallback(
        [](Connection& connection,
           std::string_view message) {
            connection.Send(message);
            connection.Send("\n");
        }
    );

    loop.loop();
}