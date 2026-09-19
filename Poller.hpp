#pragma once
#include "Channel.hpp"


#include <unordered_map>
#include <iostream>
#include <vector>
#include <functional>

enum Method{
    ADD,
    DEL,
    UPDATE
};

//只负责维护epoll，conns这种数据结构的维护不负责
//poller不负责业务逻辑

using ChannelList =
    std::vector<Channel*>;


class Poller{
    public:
    explicit Poller(int epoll_fd):
    epoll_fd_(epoll_fd){}

    //void updatePoller(int fd, Method method, Channel& channel);
    
    void poll(
    int timeout_ms,
    ChannelList& active_channels);

    //channel中拥有fd
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

    int getfd(){
        return epoll_fd_;
    }


    private:
    int epoll_fd_;
    std::unordered_map<int, Channel*> channels;//考虑换成智能指针


};