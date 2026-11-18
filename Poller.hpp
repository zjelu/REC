#pragma once
#include "Channel.hpp"


#include <unordered_map>
#include <iostream>
#include <vector>
#include <functional>
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include <system_error>

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
    Poller();   // 创建 epoll fd
    ~Poller();  // 关闭 epoll fd


     Poller(const Poller&) = delete;
    Poller& operator=(const Poller&) = delete;

    //void updatePoller(int fd, Method method, Channel& channel);
    
    void poll(
    int timeout_ms,
    ChannelList& active_channels);

    //channel中拥有fd
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    void updateEpollCtl(Status& status, int fd,std::uint32_t events_Target, Channel& channel);

    int getfd(){
        return epoll_fd_;
    }


    private:
    int epoll_fd_;
    std::unordered_map<int, Channel*> channels;//考虑换成智能指针


};