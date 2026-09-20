#pragma once
#include <optional>
#include <iostream>
 #include <sys/socket.h>

#include "Channel.hpp"
#include "data_type.hpp"

/*
我发现服务器会给connection 设置关闭回调，函数就是close_connection，
然后connection就会存储这个回调，然后到时候触发。
但是conneciton没有给client-channel设计关闭回调，原来代码里也缺少了这一块。
我的想法是，先给client-channel设计quit关闭回调，然后试图搭建一条通路直接一条龙服务。*/

class Connection{

    public:
    using MessageCallback =
        std::function<void(
            Connection&,
            std::string_view
        )>;

    using CloseCallback =
        std::function<void(Connection&)>;


    explicit Connection(Eventloop* loop,int fd):
        client_fd_(fd),
       client_channel_(fd, EPOLLET, *loop){
            client_channel_.setReadCallback(
                [this] {
                    handleRead();
                }
            );

            client_channel_.setWriteCallback(
                [this] {
                   handleWrite();
                }
            );
            
            /*client_channel_.setCloseCallback(
                [this] {
                    quit();
                }
            );*/
            client_channel_.enableReading();
        }

    void setMessageCallback(MessageCallback func);
    void setCloseCallback(CloseCallback func);

    Connection(const Connection& other) = delete;
    Connection& operator=(const Connection&) = delete;

    Connection(Connection&&) noexcept = default;
    Connection& operator=(Connection&&) noexcept = default;


    std::optional<std::string>pop_line();

    //send()：业务层入口,不等待epolout信号，直接发送
    //虽然与业务层有关，但是基本上没有什么具体的业务功能实现
    //所以耦合度小
    void Send(std::string_view data);
    
    int fd() const noexcept {
        return client_fd_;
    }
    
    Channel& channel() {
        return client_channel_;
    }

    void WillDelete(){
        will_delete = true;
    }

    private:
    bool will_delete = false;
    void handleRead();

    //等待epolout信号再发送
    void handleWrite();

    ReadResult readData();
    FlushResult flushOutput();


    int client_fd_;
    std::string inbuf;
    std::string outbuf;

    size_t write_offset = 0; 
    size_t read_offset = 0; 

    //虽然有耦合，但是从语义上来看比较合理，每种client都有自己需求的功能
     Channel client_channel_;//负责对接那个链接的系统调用I/O

    MessageCallback message_callback_;
    CloseCallback close_callback_;


};