#pragma once
#include "Poller.hpp"


class Eventloop{

    //Eventloop拥有poller，平时不断循环调用poll()，得到Channels；
    //然后再调用Channel::handleEvent()
    public:
    explicit Eventloop(Poller &poller):
    poller_(poller){}



    void loop();
    void quit();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
   

    int getEpollfd(){
        return poller_.getfd();
    }

    private:
    Poller poller_;
    ChannelList active_channels_;
    bool quit_ = false;

};