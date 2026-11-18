#include "Eventloop.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <system_error>

Eventloop::Eventloop()
    {
     wakeup_fd_ = ::eventfd(
        0,
        EFD_NONBLOCK | EFD_CLOEXEC
    );

    if (wakeup_fd_ < 0) {
        throw std::system_error(
            errno, std::generic_category(), "eventfd"
        );
    }

    try{
        wakeup_channel_ =
            std::make_unique<Channel>(wakeup_fd_, 0, *this);

        wakeup_channel_->setReadCallback([this] {
            handleWakeup();
        });

        //这一步会把wakeup_channel_加入poller中
        wakeup_channel_->enableReading();
    }catch(...){
        close(wakeup_fd_);
        wakeup_fd_=-1;
        throw;
    }

}



void Eventloop::loop() {
    quit_ = false;

    while (!quit_) {
        active_channels_.clear();

        //poll()监听的时候把所有的f系统调用带植入到active_channels_
        poller_.poll(-1, active_channels_);


//runPendingTasks() 虽然写在每轮 loop() 的末尾，
//但它只有在 poller_.poll(-1, ...) 返回之后才有机会执行
//只要 loop 成功完成这一轮，确实一定会调用 runPendingTasks()
        
    for (Channel* channel : active_channels_) {
            channel->handleEvent();
        }
        //这里就把之前所有的任务处理好
        runPendingTasks();
    }
}

void Eventloop::quit(){
    quit_=true;

}
void Eventloop::wakeup(){
    //创建 eventfd
  //→ 用 Channel 注册到本 Loop
  //→ queueTask() 写 eventfd
  //→ 唤醒 Channel 的读回调读取 eventfd

  const std::uint64_t one = 1;
    ssize_t n = write(wakeup_fd_,&one,sizeof(one));

    if(n == static_cast<ssize_t>(sizeof(one))) return;
    if (n < 0 && errno == EAGAIN) {
        return;
    }

    throw std::system_error(
        n < 0 ? errno : EIO,
        std::generic_category(),
        "eventfd write"
    );

}

void Eventloop::handleWakeup(){
    std::uint64_t count;
    ssize_t n = read(wakeup_fd_, &count, sizeof(count));
    if(n == static_cast<ssize_t>(sizeof(count))) return;
    if (n < 0 && errno == EAGAIN) {
        return;
    }

    throw std::system_error(
        n < 0 ? errno : EIO,
        std::generic_category(),
        "eventfd read"
    );
}

void Eventloop::updateChannel(Channel* channel){

    //std::cerr<<"to update channel in Eventloop\n";
    poller_.updateChannel(channel);
}

void Eventloop::removeChannel(Channel* channel){
        poller_.removeChannel(channel);

}

void Eventloop::runPendingTasks(){
   std::vector<std::function<void()>> tasks;

    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        tasks.swap(pending_tasks_);
    }

    for (auto& task : tasks) {
        task();
    }
    //pending_tasks_.clear();
}

void Eventloop::queueTask(std::function<void()> task){
    {
        std::lock_guard<std::mutex> lock(tasks_mutex_);
        pending_tasks_.push_back(std::move(task));
    }

    wakeup();  // 写 eventfd，唤醒所属线程
   }
