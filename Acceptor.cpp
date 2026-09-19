#include "Acceptor.hpp"
int Acceptor::getNonblockClientfd(){
        while(true){
        struct sockaddr_in client;
        socklen_t len = sizeof(client);

        int clientsock= accept(listen_fd_,(struct sockaddr*)&client, &len);

        if(clientsock <0){
            if(errno == EINTR){
                continue;
            }
            if(errno==EAGAIN || errno ==EWOULDBLOCK){
                break;
            }
            perror("accept");
            break;
        }

        if (setnonblocking(clientsock) == -1) {
            ::close(clientsock);       
            continue;
            } 

            return clientsock;
        }
        return -1;
    }

int Acceptor::setnonblocking(int fd){
        int flags = fcntl(fd, F_GETFL, 0);

        if(flags == -1)
        {
            return -1;
        }

        return fcntl(
            fd,
            F_SETFL,
            flags | O_NONBLOCK
        );
    }

void Acceptor::handleAccept(
    Eventloop* loop){

    while (true) {
        int client_fd = accept4(
            listen_fd_,
            nullptr,
            nullptr,
            SOCK_NONBLOCK | SOCK_CLOEXEC
        );

        if (client_fd >= 0) {
            if (new_connection_callback_) {
                new_connection_callback_(client_fd);
            } else {
                // 没有人接管，避免泄漏
                ::close(client_fd);
            }
            continue;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN ||
            errno == EWOULDBLOCK) {
            break;
        }

        std::perror("accept4");
        break;
    } 
}
