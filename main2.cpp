#include "Acceptor.hpp"
#include "unordered_map"
#include "Eventloop.hpp"
#include "Connection.hpp"
#include "Tcpserver_thread.hpp"

#include <sys/socket.h>
#include <sys/eventfd.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unistd.h>


int main(int argc, char* argv[]) {

     if (argc < 2) {
        std::cerr
            << "usage: ./server <port> [port ...]\n";

        return 1;
    }
    const int port = std::stoi(argv[1]);

    if (port < 1 || port > 65535) {
        std::cerr
            << "invalid port: "
            << argv[1]
            << '\n';

        return 1;
    }

    

    Eventloop main_loop;
    Tcpserver server(port, &main_loop);

    server.run();
}