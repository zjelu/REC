#include "Connection.hpp"

std::optional<std::string> Connection::pop_line(){
      
       const std::size_t pos = inbuf.find('\n',read_offset);
        if(pos== std::string::npos)
        {
            inbuf.erase(0,read_offset);
            read_offset=0;

            return std::nullopt;
        }

        std::string line(
            inbuf.data()+read_offset,
            pos- read_offset
            
        );

        read_offset=pos+1;
        return line;
}

/*不过建议不要直接调用 readData() 和 flushOutput()，
而是增加两个负责状态协调的成员函数：*/
ReadResult Connection::readData(){
     char buf[4096];
        while(true){
            const ssize_t n = recv(client_fd_,buf,sizeof(buf),0);
            if(n>0){
                inbuf.append(buf,static_cast<std::size_t>(n));
                continue;
            }
            if(n==0){
                //close(client_fd_);
                std::cerr
                << "peer closed, client_fd_="
                << client_fd_
                << '\n';

                return ReadResult::PeerClosed;
            }
            if(errno == EINTR){
                continue;
            }

            if(errno == EAGAIN || errno == EWOULDBLOCK){

                return ReadResult::Ready;
            }

            if (errno != EAGAIN &&
                errno != EWOULDBLOCK &&
                errno != EINTR) {
                std::cerr
                    << "read error, client_fd_="
                    << client_fd_
                    << '\n';

            return ReadResult::Error; 
            }
        }    
}

 FlushResult Connection::flushOutput(){
     {
        while (write_offset < outbuf.size()) {
            ssize_t result = send(
            client_fd_,
            outbuf.data() + write_offset,
            static_cast<size_t>(outbuf.size() - write_offset),
            MSG_NOSIGNAL
        );

        if (result > 0) {
        write_offset += static_cast<std::size_t>(result);
            continue;
        }

        if (result < 0 && errno == EINTR) {
            continue;
        }

        if (result < 0 &&
            (errno == EAGAIN || errno == EWOULDBLOCK)) {

            //outbuf.erase(0, sent);//性能存疑
            return FlushResult::WouldBlock;
        }

        perror("send http");
            return FlushResult::Error;
        }

        outbuf.clear();
        write_offset = 0;

        return FlushResult::Done;
       // printf("[WRITE] client_fd_=%d  total=%zu", client_fd_,  conn.output_buffer.size());
        }
    }

/*readData()：socket → inbuf
pop_line()：inbuf → 完整消息
messageCallback：完整消息 → 上层业务*/

//handleread函数是传给channel来执行功能的
void Connection::handleRead() {
    const ReadResult result = readData();

    if (result == ReadResult::PeerClosed ||
        result == ReadResult::Error) {
        if (close_callback_) {
            close_callback_(*this);//requestConnection()触发，标记为将要删除
        }
        return;
    }

    while (auto message = pop_line()) {
        if (message_callback_) {
            message_callback_(*this, *message);
        }
    }
}

//主动发送路径
void Connection::Send(std::string_view data) {
    const bool was_empty = outbuf.empty();

    outbuf.append(data.data(), data.size());

    // 原来已经有数据等待发送，
    // 说明 Connection 正在等待 EPOLLOUT。
    if (!was_empty) {
        return;
    }

    const FlushResult result = flushOutput();

    if (result == FlushResult::WouldBlock) {
        client_channel_.enableWriting();
        return;
    }

    if (result == FlushResult::Error) {
        if (close_callback_) {
            close_callback_(*this);//requestConnection()触发，标记为将要删除
        }
    }
}

//EPOLLOUT 续写路径
void Connection::handleWrite() {
    const FlushResult result = flushOutput();

    if (result == FlushResult::Done) {
        client_channel_.disableWriting();
        return;
    }

    if (result == FlushResult::Error) {
        if (close_callback_) {
            close_callback_(*this);
        }
    }

    // WouldBlock：什么都不做，
    // 继续保持对 EPOLLOUT 的关注。
}

void Connection::setMessageCallback(MessageCallback func){
  message_callback_=func;
}
void Connection::setCloseCallback(CloseCallback func){
  close_callback_=func;
};