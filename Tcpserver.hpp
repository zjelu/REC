#pragma once
#include "Acceptor.hpp"
#include "unordered_map"
#include "Eventloop.hpp"
#include "Connection.hpp"
#include <memory>

class Tcpserver{
    public:
    using MessageCallback = Connection::MessageCallback;

    Tcpserver(int port, Eventloop* eventloop);
    
    void setMessageCallback(MessageCallback callback) {
        message_callback_ = std::move(callback);
    }
    


    private:
    void newConnection(int client_fd);
    void removeConnection(Connection& connection);

    //简单的功能函数，暂时放在Tcpserver中
    void echoMessage(
        Connection& connection,
        std::string_view message)
    {
        connection.Send(message);
        connection.Send("\n");
    }

    int port_;

    std::unique_ptr<Acceptor> acceptor_;
    
    Eventloop* eventloop;

    std::unordered_map<int,std::unique_ptr<Connection>> connections;

    MessageCallback message_callback_;



};