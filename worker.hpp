#pragma once
#include <memory>
#include <thread>
#include <unordered_map>
#include "Connection.hpp"
#include "Eventloop.hpp"

class Worker{
    public:
     Worker()=default;

    void addconnection(int fd);
   
    void requestConnection(Connection& connection);
    void newConnection(int client_fd);
    void removeConnection(Connection& connection);
    void removeConnectionByFd(int fd);

    void setMessageCallback(Connection::MessageCallback func){
        message_callback_ = func;
    }

    void start();
    void stop();

    
    private:
    //void requestStop();
    //void join();

    Eventloop loop_;
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    std::thread thread_;
    Connection::MessageCallback message_callback_;
};