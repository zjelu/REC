#include <gtest/gtest.h>

#include <memory>
#include <vector>
#include <sys/epoll.h>
#include <unistd.h>

#include "Eventloop.hpp"
#include "Poller.hpp"

// 每个 TEST_F 都会创建独立的环境，执行后清理。
class EventloopTest : public ::testing::Test {
protected:
    void SetUp() override {
        epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
        ASSERT_GE(epoll_fd_, 0);

        Poller poller(epoll_fd_);
        loop_ = std::make_unique<Eventloop>(poller);
    }

    void TearDown() override {
        loop_.reset();

        if (epoll_fd_ >= 0) {
            ::close(epoll_fd_);
        }
    }

    int epoll_fd_ = -1;
    std::unique_ptr<Eventloop> loop_;
};

// 入队不应立即执行；显式处理时执行一次。
TEST_F(EventloopTest, QueuedTaskRunsOnlyWhenDrained) {
    int calls = 0;

    loop_->queueTask([&] {
        ++calls;
    });

    EXPECT_EQ(calls, 0);

    loop_->runPendingTasks();
    EXPECT_EQ(calls, 1);

    loop_->runPendingTasks();
    EXPECT_EQ(calls, 1);
}

// 同一批任务按入队顺序执行。
TEST_F(EventloopTest, TasksRunInQueueOrder) {
    std::vector<int> order;

    loop_->queueTask([&] { order.push_back(1); });
    loop_->queueTask([&] { order.push_back(2); });

    loop_->runPendingTasks();

    EXPECT_EQ(order, (std::vector<int>{1, 2}));
}

// 执行过程中新增的任务，留到下一批处理。
TEST_F(EventloopTest, NewlyQueuedTaskRunsInNextBatch) {
    std::vector<int> order;

    loop_->queueTask([&] {
        order.push_back(1);

        loop_->queueTask([&] {
            order.push_back(2);
        });
    });

    loop_->runPendingTasks();
    EXPECT_EQ(order, (std::vector<int>{1}));

    loop_->runPendingTasks();
    EXPECT_EQ(order, (std::vector<int>{1, 2}));
}