#pragma once
#include <cstdint>
#include <functional>
#include <sys/epoll.h>

//Channel.hpp 使用前向声明，阻止循环引用
class Eventloop;

class Channel{

    public:
    //在channel构造好的时候直接塞进loop中
        explicit Channel(int fd, uint32_t events, Eventloop& loop):
        loop_(&loop),
        fd_(fd),
        events_(events){
            //把channel负责加入epoll_ctl（不需要了，已经有update函数了）
            //loop_.updateChannel(this);//编译存疑，毕竟不算构造完全
        };

        void handleEvent();

        //负责设置回调函数，一般来说listenfd或clientfd来消息的时候，他们会被调用
        void setReadCallback(std::function<void()>func);
        void setWriteCallback(std::function<void()>func);
        void setCloseCallback(std::function<void()>func);
        void setErrorCallback(std::function<void()>func);

        //channel函数禁止复制
        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;

        void setRevents(uint32_t revents){
        revents_ = revents;
        }
        int fd() const noexcept{
            return fd_;
        }
        std::uint32_t events() const noexcept{
            return events_;
        }
        std::uint32_t revents() const noexcept{
            return revents_;
        }

        void enableReading();
        void enableWriting();
        void disableWriting();
        void disableAll();


    private:
        void update();
        
        Eventloop* loop_;//// 借用，不拥有
        int fd_;  //有listenfd,也有clientfd,但是channel不需要知道
        std::uint32_t events_;//我关心是什么，他只要知道这个事件可读还是可写
        std::uint32_t revents_;//真实的events


        std::function<void()> read_callback_;
        std::function<void()> write_callback_;
        std::function<void()> close_callback_;
        std::function<void()> error_callback_;

};