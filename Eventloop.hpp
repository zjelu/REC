#pragma once
#include "Poller.hpp"
#include <cassert>
#include <mutex>
#include <memory>
#include <unistd.h>

class Eventloop{

    //Eventloop拥有poller，平时不断循环调用poll()，得到Channels；
    //然后再调用Channel::handleEvent()
    public:
     Eventloop();
    ~Eventloop(){
        /*- Poller 知道有哪些 Channel
        - 但它不负责 delete Channel
        - 真正拥有 wakeup_channel_ 的是 EventLoop 里的 unique_ptr
        所以即使 Poller 马上也要析构，它也不会、
        也不应该去释放 wakeup_channel_。
        否则就会和 EventLoop 的 unique_ptr 形成双重释放风险
        */


        if(wakeup_channel_){
            removeChannel(wakeup_channel_.get());
            wakeup_channel_.reset();
        }
        if (wakeup_fd_ >= 0) {
            ::close(wakeup_fd_);
        }
    }

    void loop();
    void quit();
    void wakeup();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    void handleWakeup();

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
    
    private:
    std::mutex tasks_mutex_; 
    int wakeup_fd_;   //用于跨线程唤醒以及沟通
    std::unique_ptr<Channel> wakeup_channel_;//要把这个channel加入poller，这样就有一个专门负责沟通的了
    

};