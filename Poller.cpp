#include "Poller.hpp"
#include "Channel.hpp"//因为真实调用需要channel里面的函数

//Poller 不应该直接执行回调。执行回调属于 EventLoop 的职责
void Poller::updateEpollCtl(Status& status, int fd,std::uint32_t events_Target, Channel& channel)
{
    struct epoll_event ev;
    ev.events = events_Target;
    ev.data.fd = fd;

    if(status == Status::ToAdd){
        if(epoll_ctl(
            epoll_fd_,
            EPOLL_CTL_ADD,
            fd,
            &ev)==-1){
            std::cerr<<"EPOLL_CTL_ADD failed\n";
            return;
            }
        channels[fd] = &channel;
    }

    if(status == Status::ToWrite){
        if(epoll_ctl(
            epoll_fd_,
            EPOLL_CTL_MOD,
            fd,
            &ev)==-1){
            std::cerr<<"EPOLL_CTL_MOD failed\n";
            return;
            }
    }

    if(status == Status::TodisableWrite){
        if(epoll_ctl(
            epoll_fd_,
            EPOLL_CTL_MOD,
            fd,
            &ev)==-1){
            std::cerr<<"EPOLL_CTL_MOD failed\n";
            return;
            }
    }

    if(status == Status::ToQuit){
        if(epoll_ctl(
            epoll_fd_,
            EPOLL_CTL_DEL,
            fd,
            &ev)==-1){
            std::cerr<<"EPOLL_CTL_DEL failed\n";
            return;
            }
        auto it = channels.find(fd);

        if(it != channels.end()){
            channels.erase(it);

        } 
    }
}

void Poller::updateChannel(Channel* channel){

    std::uint32_t events_Target=channel->events();//获取channel想要的events
    std::uint32_t events_Real=channel->revents();//获取channel想要的events
    Status status= channel->status();
    int fd = channel->fd();

       
        updateEpollCtl(status, fd, events_Target, *channel);
        //channel->setRevents(events_Target);不需要，只能有epoll_wait来实现
        
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

