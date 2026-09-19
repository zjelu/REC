#pragma once
#include <cerrno>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
 
#include "Channel.hpp"
#include "Eventloop.hpp"

//Acceptor归属于server，但是拥有listenfd
//负责 socket/bind/listen/accept

using NewConnectionCallback =
    std::function<void(int)>;


class Acceptor{

    public:
    //创建了一个listen_channel，然后将它与listen_fd绑定，然后注册进入eventloop
    //不过似乎不需要注册进connections，因为后者是负责clientsock的
    explicit Acceptor(int listen_fd, Eventloop* loop):
    listen_fd_(listen_fd),
    listen_channel_(listen_fd,EPOLLET,*loop),
    loop_(loop){
    
        //设置为非阻塞
    if(setnonblocking(listen_fd) == -1){
        std::cerr<< "setnonblocking failed"<<std::endl;
        return;//程序退出了
    }

        listen_channel_.setReadCallback(
            [this]{
                handleAccept(loop_);//构造的时候就setReadCallback，为了将来的调用做准备
            }
        );


        listen_channel_.enableReading();
    }

    //通过listen_fd来监听，然后创建对应的client_fd
    //不负责维护epoll与connections的功能


    int  getNonblockClientfd();
    int  setnonblocking(int fd);
    void setNewConnectionCallback(
            NewConnectionCallback callback)
        {
            new_connection_callback_ =
                std::move(callback);
        }
        
    // 参与构成handlEvent的一部分
    // 不过需要传入Eventloop的poller的epoll_fd
    // 与server的Connections
    // 或许后续可以考虑优化

    void handleAccept(
        Eventloop* loop);

    
    private:
    int listen_fd_;
    Channel listen_channel_;//里面塞得是与listen有关的回调函数
    Eventloop* loop_;
    NewConnectionCallback new_connection_callback_;//用来维护connection的拓展
    
};