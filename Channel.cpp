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

void Channel::handleEvent(
) {
    if (revents_ & EPOLLERR) {
        if (error_callback_) {
            error_callback_();
        }
    }

    if (revents_ & EPOLLHUP) {
        if (close_callback_) {
            close_callback_();
        }

        return;
    }

    if (revents_ &
        (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (read_callback_) {
            read_callback_();
        }
    }

    if (revents_ & EPOLLOUT) {
        if (write_callback_) {
            write_callback_();
        }
    }
}

void Channel::update(){
    std::cerr<<"to update channel\n";
    loop_->updateChannel(this);
}

void Channel::enableReading()
{
    events_ |= EPOLLIN;
    update();
}

void Channel::enableWriting()
{
    events_ |= EPOLLOUT;
    update();
}

void Channel::disableWriting()
{
    events_ &= ~EPOLLOUT;
    update();
}

