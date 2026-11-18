#pragma once
#include "Acceptor.hpp"
#include "unordered_map"
#include "Eventloop.hpp"
#include "Connection.hpp"
#include "worker.hpp"
#include <memory>
#include <vector>

class Tcpserver{
    public:
    using MessageCallback = Connection::MessageCallback;

    Tcpserver(int port, Eventloop* eventloop);
    
    void setMessageCallback(MessageCallback callback) {
        message_callback_ = std::move(callback);
    }
    void run();
    


    private:
    void newConnection(int client_fd);
    //void removeConnection(Connection& connection);
    //void removeConnectionByFd(int fd);

    //简单的功能函数，暂时放在Tcpserver中
    void echoMessage(
        Connection& connection,
        std::string_view message)
    {
        /*std::cout << "[Echo] 处理消息"
          << " fd=" << connection.fd()
          << " thread=" << std::this_thread::get_id()
          << std::endl;*/

        std::string response(message);
        response.push_back('\n');
        connection.Send(response);
    }

    int port_;
    std::size_t next_worker_ = 0;

    std::unique_ptr<Acceptor> acceptor_;
    Eventloop* mainloop;
    std::vector<std::unique_ptr<Worker>> workers_;
    //Worker* worker;
    MessageCallback message_callback_;

};