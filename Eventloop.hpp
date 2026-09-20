#pragma once
#include "Poller.hpp"
#include <cassert>

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

    void runPendingTasks();
    void queueTask(std::function<void()> task);
    
    private:
    
    Poller poller_;
    ChannelList active_channels_;
    bool quit_ = false;
    std::vector<std::function<void()>> pending_tasks_;//一排以后要执行的任务

    

};