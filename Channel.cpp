#include "Channel.hpp"
#include "Eventloop.hpp"

void Channel::setReadCallback(std::function<void()>func){
    read_callback_=func;

}
void Channel::setWriteCallback(std::function<void()>func){
  write_callback_=func;
}

void Channel::setCloseCallback(std::function<void()>func){
  close_callback_=func;
}
void Channel::setErrorCallback(std::function<void()>func){
  error_callback_=func;
};

//但是关闭channle要再handelevent中进行
void Channel::handleEvent(
) {
    /*std::cerr << "[Event] fd=" << fd_
          << " revents=" << revents_
          << " has_read=" << static_cast<bool>(read_callback_)
          << " has_close=" << static_cast<bool>(close_callback_)
          << " has_error=" << static_cast<bool>(error_callback_)
          << '\n';*/

    if (revents_ & EPOLLERR) {
        if (error_callback_) {
            error_callback_();
        }
    }

    //猜测这条路径没有被调用
    if (revents_ & EPOLLHUP) {
        if (close_callback_) {
            //std::cerr<<"revents EPOLLHUP \n";
            close_callback_();
        }
        return;
    }

    if (revents_ &
        (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) {
            /*std::cerr << "[Channel read] fd=" << fd_
                  << " revents=" << revents_
                  << '\n';*/
            read_callback_();
        }
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
}

//既然要实现灵活多变的话，那么我选择让EPOLL查看channel希望获得的events,然后改变内核状态
void Channel::update(){
    //std::cerr<<"to update channel\n";
    loop_->updateChannel(this);
}

void Channel::quit(){
    //std::cerr<<"to quit channel\n";
    stat = Status::ToQuit;
    is_quit = true;
    update();
}



//一般来说只有创建初始的时候才会enableReading一次
void Channel::enableReading()
{
    events_ |= EPOLLIN;
    stat = Status::ToAdd;
    update();
    
}

void Channel::enableWriting()
{
    events_ |= EPOLLOUT;
     stat = Status::ToWrite;
    update();
}

void Channel::disableWriting()
{
    events_ &= ~EPOLLOUT;
    stat = Status::TodisableWrite;
    update();
}

