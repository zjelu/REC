#include "Eventloop.hpp"

void Eventloop::loop() {
    quit_ = false;

    while (!quit_) {
        active_channels_.clear();

        poller_.poll(-1, active_channels_);

        for (Channel* channel : active_channels_) {
            channel->handleEvent();
        }
    }
}

void Eventloop::quit(){
    quit_=true;

}

void Eventloop::updateChannel(Channel* channel){

    std::cerr<<"to update channel in Eventloop\n";
    poller_.updateChannel(channel);
}

void Eventloop::removeChannel(Channel* channel){
        poller_.removeChannel(channel);

}
