#include "worker.hpp"

/*
cmake --build Reactor/build-thread \
    --target reactor_server_thread -j*/


//addconnection是连接的时候的跨线程关键
//给worke的loop的任务队列塞入创建一个具体线程的流程
void Worker::addconnection(int fd){
    loop_.queueTask([this,fd]{

       /*std::cout << "[Worker] 创建连接"
                  << " fd=" << fd
                  << std::endl;
                  //<< " thread=" << std::this_thread::get_id()*/

            // 这里由工作线程执行
             auto connection =
            std::make_unique<Connection>(&loop_, fd);

           connection->setMessageCallback(message_callback_);

            //原来的关闭链基本没变，只是最终管理者换了
            connection->setCloseCallback(
                [this](Connection& connection) 
                {
                requestConnection(connection);
                }
            );
            connections_.emplace(fd, std::move(connection));
        });
}

//还没有写函数调用start()
void Worker::start(){
     thread_ = std::thread([this] {
        loop_.loop();
    });
}

void Worker::stop(){
     if (!thread_.joinable()) {
        return;
    }

    // 将退出操作放进工作线程的任务队列。
    loop_.queueTask([this] {
        loop_.quit();
    });

    // 等待工作线程从 loop() 返回。
    thread_.join();
}

void Worker::requestConnection(Connection& connection) {
    connection.channel().quit();

    const int fd = connection.fd();

    loop_.queueTask([this, fd] {
        connections_.erase(fd);
    });
}

//这个函数目前没有被调用过
void Worker::removeConnectionByFd(int fd)
{
    auto it = connections_.find(fd);
    if (it == connections_.end()) {
        return;
    }

    removeConnection(*it->second);
}

//这个函数目前没有被调用过
void Worker::removeConnection(
    Connection& connection)
{
    const int fd = connection.fd();

    auto it = connections_.find(fd);

    if (it == connections_.end()) {
        std::cerr
            << "failed to find and remove "
            << fd
            << '\n';
        return;
    }
     connections_.erase(it);
}