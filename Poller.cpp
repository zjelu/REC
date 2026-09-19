#include "Poller.hpp"
#include "Channel.hpp"//因为真实调用需要channel里面的函数

//Poller 不应该直接执行回调。执行回调属于 EventLoop 的职责

void Poller::updateChannel(Channel* channel){
    epoll_event cev;
            int fd = channel->fd();
            cev.data.fd=fd;
            cev.events=EPOLLIN|EPOLLET;//只接受读事件,意味着有内容的时候会通知，不代表不能够发送内容

            if(epoll_ctl(
                epoll_fd_,
                EPOLL_CTL_ADD,
                fd,
                &cev)==-1)
            {
                //close(fd);
                //新添fd与对应的回调函数
                std::cerr<<"EPOLL_CTL_ADD failed\n";
                return;
            }

            channels[fd]=channel;
}

void Poller::removeChannel(Channel* channel){
     int fd = channel->fd();
     epoll_ctl(
                epoll_fd_,
                EPOLL_CTL_DEL,
                fd,
                nullptr
            );
            std::cerr<<"EPOLL_CTL_DEL successfully\n";

            auto it = channels.find(fd);
            if(it == channels.end()) {
                std::cerr<<"failed to find  DEL fd in channels\n";
                return;
            }

        channels.erase(it);
}
    
void Poller::poll(int timeout_ms,
    ChannelList& active_channels
){
   
        std::array<epoll_event, 10> ready_events{};
        int ready_count = 
        epoll_wait(
            epoll_fd_,
            ready_events.data(),
            static_cast<int>(ready_events.size()),
            -1// 第一版先用短超时
        );

        if(ready_count ==-1){
            if(errno == EINTR) return;
            else{
                throw std::runtime_error(
                    std::string("epoll_wait failed")
                );
            };
        }

        for(int i=0;i<ready_count;++i){
            const int fd = 
                ready_events[i].data.fd;

            const std::uint32_t events =
                ready_events[i].events;

            auto it = channels.find(fd);
            if(it == channels.end()) continue;

            Channel* channel=it->second;
            channel->setRevents(events);
            active_channels.push_back(channel);
    }
}

