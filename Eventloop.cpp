#include "Eventloop.hpp"

void Eventloop::loop() {
    quit_ = false;

    while (!quit_) {
        active_channels_.clear();

        poller_.poll(-1, active_channels_);

        for (Channel* channel : active_channels_) {
            channel->handleEvent();
            //这时候再彻底的removeconnection吗？
        }
        runPendingTasks();
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

void Eventloop::runPendingTasks(){
    std::vector<std::function<void()>> tasks;
    tasks.swap(pending_tasks_);

    for (auto& task : tasks) {
        task();
    }

    //pending_tasks_.clear();
}

void Eventloop::queueTask(std::function<void()> task){
    pending_tasks_.push_back(std::move(task));
   }
