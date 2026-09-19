#include <cstdint>
#include "Channel.cpp"
#include "Poller.cpp"

#include "Eventloop.cpp"
#include <sys/socket.h>
#include <sys/eventfd.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <unistd.h>

int main(){
    

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

    Channel channel(event_fd, EPOLLET, loop);

    channel.setReadCallback([&] {
        std::uint64_t value = 0;

        if (read(event_fd, &value, sizeof(value)) == -1) {
            std::perror("read eventfd");
            return;
        }

        std::cout << "eventfd callback, value="
                  << value << '\n';

        loop.quit();
    });


    channel.enableReading();

    std::uint64_t value = 1;
    if (write(event_fd, &value, sizeof(value)) == -1) {
        std::perror("write eventfd");
        return 1;
    }

    loop.loop();

    close(event_fd);
    close(epoll_fd);

}