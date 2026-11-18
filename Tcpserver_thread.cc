#include "Tcpserver_thread.hpp"

//newConnection被塞入了mainloop中
//关键理解addconnection的跨线程调用
void Tcpserver::newConnection(int client_fd) {
    /*std::cout << "[Main] 接收连接"
            << " fd=" << client_fd
            << std::endl;*/

     //其实不仅仅时候创建一个新的connection，也是初始化他
    auto& worker = workers_[next_worker_];
    next_worker_ = (next_worker_ + 1) % workers_.size();
    worker->addconnection(client_fd);


}

 Tcpserver::Tcpserver(int port, Eventloop* mainloop):
    port_(port),
    mainloop(mainloop)
    {
    
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if(sock<0){
        perror("socket() failed");
        return;
    }

    int opt =1;
    unsigned int len = sizeof(opt);
    setsockopt(sock, SOL_SOCKET,SO_REUSEADDR,&opt,len);

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(static_cast<std::uint16_t>(port_));

    if (bind(sock,(struct sockaddr*)& servaddr,sizeof(servaddr)) < 0)
    {
        perror("bind() failed");
        return;
    }

     if (listen(sock, 128) < 0) {
        const int saved_errno = errno;
        ::close(sock);

        throw std::system_error(
            saved_errno,
            std::generic_category(),
            "listen"
        );
    }

    int listen_fd = sock;

     printf("[LISTEN] fd=%d port=%d\n",
        listen_fd,
        port);

    //-----------------------------------------
    //用获得的listen_fd来构造acceptor
    //getconnections()直接默认构造了，小心[fd]调用不存在的默认构造函数陷阱

    //创建一个listenfd，然后将它与mainloop绑定起来
    acceptor_ =
    std::make_unique<Acceptor>(
        listen_fd,
        mainloop
    );

    //这里没有要求 Server 去自己的容器中查找 Connection。
    // 实际触发回调时，Connection 会把自己传进来。比如message_callback_(*this, *message);
    acceptor_->setNewConnectionCallback(
        [this](int client_fd) {
            newConnection(client_fd);
        }
    );

    //worker创建的时候就设置了setMessageCallback具体的
   workers_.reserve(4);

for (int i = 0; i < 4; ++i) {
    auto worker = std::make_unique<Worker>();

    worker->setMessageCallback(
        [this](Connection& connection,
               std::string_view message) {
            echoMessage(connection, message);
        }
    );

    workers_.push_back(std::move(worker));
}

}

void Tcpserver::run() {
    try {
        for (auto& worker : workers_) {
            worker->start();
        }

        mainloop->loop();
    } catch (...) {
        for (auto& worker : workers_) {
            worker->stop();
        }
        throw;
    }

    for (auto& worker : workers_) {
        worker->stop();
    }
}

