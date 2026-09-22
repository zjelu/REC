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

    void removeConnectionByFd(int fd);

    void requestConnection(Connection& connection)
    {

        //connection.WillDelete();没有必要
        connection.channel().quit();

        int fd = connection.fd();

        eventloop->queueTask(
            [this, fd] {
                removeConnectionByFd(fd);
            }
        );
        //状态设置成willdelete,但是不能够真的删除
        //因为这将被我们作为channel::handle(connection的close回调函数)，
        //删除了connection，channel就没了，但是channel函数才运行到一半
    }
    

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
    //std::vector<Connection> derredConnections;//不需要，luup
    MessageCallback message_callback_;

};