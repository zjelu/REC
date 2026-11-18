#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

/*
如何实现线程交接呢？

主线程
  → 把函数放入 pending_tasks
  → write(eventfd)
        ↓
工作线程的 epoll_wait 返回
  → read(eventfd)
  → 取出 pending_tasks
  → 执行函数

  模拟一个线程向另一个线程发送消息
  要注意什么地方呢？
*/

/*
主线程：
  向工作 Loop 的 pending_tasks_ 放入任务
  → write(wakeup_fd)，发送通知

工作线程：
  epoll_wait() 发现 wakeup_fd 可读
  → read(wakeup_fd)，消费通知
  → 从 pending_tasks_ 取出任务
  → task()
  
  发了任务还有顺便通知对面
  */

using Task = std::function<void()>;
void check(bool ok, const char* operation) {
    if (!ok) {
        std::perror(operation);
        std::exit(EXIT_FAILURE);
    }
}

int main(){
    //在主线程中创建一个epoll_fd
    const int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    check(epoll_fd >= 0, "epoll_create1");

    //在主线程创建一个eventfd(wake_fd)
    const int wakeup_fd =
        eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    check(wakeup_fd >= 0, "eventfd");


    epoll_event registration{};
    registration.events = EPOLLIN;
    registration.data.fd = wakeup_fd;

    check(
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD,
                  wakeup_fd, &registration) == 0,
        "epoll_ctl"
    );

    //pending_tasks属于主线程
    std::mutex mutex;
    std::vector<Task> pending_tasks;

    //只由工作线程访问
    bool stopping = false;

    auto queueTask = [&](Task task){
        {
            std::lock_guard<std::mutex> lock(mutex);
            pending_tasks.push_back(std::move(task));
        }
        
        // 数据存进队列后，再通知工作线程。
        const std::uint64_t one = 1;
        ssize_t result;
        
    //主线程向工作线程发送数据
        do{
            result = write(wakeup_fd,&one,sizeof(one));
        }while(result < 0 && errno ==EINTR);
        
        // EAGAIN 表示计数器已满，此时 fd 已经可读。
        check(
            result == static_cast<ssize_t>(sizeof(one)) ||
            (result < 0 && errno == EAGAIN),
            "write eventfd"
        );
    };

    //初始化工作线程，似乎同时也让它执行 work
    std::thread worker([&] {
        while (!stopping) {
            epoll_event ready{};
            int count;

            do {
                count = epoll_wait(epoll_fd, &ready, 1, -1);
            } while (count < 0 && errno == EINTR);

            check(count >= 0, "epoll_wait");

            // 读取通知，清空 eventfd 的累计计数。
            std::uint64_t notifications;
            ssize_t result;

            //一直读，感觉很简洁
            do {
                result = ::read(
                    wakeup_fd, &notifications,
                    sizeof(notifications)
                );
            } while (result < 0 && errno == EINTR);

            check(
                result ==
                    static_cast<ssize_t>(sizeof(notifications)) ||
                    (result < 0 && errno == EAGAIN),
                "read eventfd"
            );

            //这里工作线程从主线程的pending_tasks获取任务并执行
            std::vector<Task> tasks;
            {
                std::lock_guard<std::mutex> lock(mutex);
                tasks.swap(pending_tasks);
            }

            // 执行任务时不持有队列锁。
            for (auto& task : tasks) {
                task();
            }
        }
    }); 

    //获取主线程的id
    const auto main_thread = std::this_thread::get_id();

    queueTask([main_thread] {
        std::cout
            << "submitted from: " << main_thread << '\n'
            << "executed on:    "
            << std::this_thread::get_id() << '\n';
    });

    // 退出也交给工作线程执行，避免跨线程修改 stopping。
    //似乎塞入pending_tasks都会交给工作线程执行
    queueTask([&] {
        stopping = true;
    });

    worker.join();

    ::close(wakeup_fd);
    ::close(epoll_fd);

    
}