#include "Tcpserver.hpp"

void Tcpserver::newConnection(int client_fd) {
    auto connection =
        std::make_unique<Connection>(
            eventloop,
            client_fd
        );

    connection->setMessageCallback(
        [this](Connection& connection,
               std::string_view message) {
            echoMessage(connection, message);
        }
    );

    connection->setCloseCallback(
    [this](Connection& connection) {
        requestConnection(connection);
    }
    //这里的关闭回调设计了requestConnection
    //到时候handleread()触发关闭回调的时候机会自动触发该函数
    //然后handleEvent结束的时候就会
    );

    connections.emplace(
        client_fd,
        std::move(connection)
    );
}



void Tcpserver::removeConnection(
    Connection& connection)
{
    const int fd = connection.fd();

    auto it = connections.find(fd);

    if (it == connections.end()) {
        std::cerr
            << "failed to find and remove "
            << fd
            << '\n';
        return;
    }

    connections.erase(it);
}



 Tcpserver::Tcpserver(int port, Eventloop* eventloop):
    port_(port),
    eventloop(eventloop){
    
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
    //Connections直接默认构造了，小心[fd]调用不存在的默认构造函数陷阱
    acceptor_ =
    std::make_unique<Acceptor>(
        listen_fd,
        eventloop
    );

    acceptor_->setNewConnectionCallback(
        [this](int client_fd) {
            newConnection(client_fd);
        }
    );

}

void Tcpserver::removeConnectionByFd(int fd)
{
    auto it = connections.find(fd);
    if (it == connections.end()) {
        return;
    }

    removeConnection(*it->second);
}