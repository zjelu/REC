//理解测试夹具，大概就是为了测试作出的基本准备
/*比如
TEST(ConnectionTest, ReceivesOneLine) {
    // 1. 准备
    // 创建 epoll、Eventloop、socketpair 和 Connection
    // 设置消息回调，记录收到的消息

    // 2. 执行
    // 对端发送 "hello\n"
    // 让 Connection 处理读事件

    // 3. 检查并清理
    // 检查回调是否收到 "hello"
    // 注销 Channel，销毁 Connection，释放其他资源

    第一第三步安全可以抽离，共性与变性分析（？）
}*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Connection.hpp"
#include "Eventloop.hpp"
#include "Poller.hpp"

class ConnectionTest : public:: testing::Test{
    public:
        void SetUp() override{
            epoll_fd_ = ::epoll_create1(EPOLL_CLOEXEC);
        ASSERT_GE(epoll_fd_, 0);

        Poller poller(epoll_fd_);
        loop_ = std::make_unique<Eventloop>(poller);

        int sockets[2];
        ASSERT_EQ(
            ::socketpair(
                AF_UNIX,
                SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                0,
                sockets
            ),
            0
        );

          connection_ =
            std::make_unique<Connection>(loop_.get(), sockets[0]);

        // 测试负责关闭另一端。
        peer_fd_ = sockets[1];

        connection_->setMessageCallback(
            [this](Connection&, std::string_view message) {
                // 必须复制，不能保存临时消息的 string_view。
                messages_.emplace_back(message);
            }
        );
    }

    void TearDown() override{
         if (connection_) {
            // 先注销，再销毁，避免 Poller 留下悬空指针。
            loop_->removeChannel(&connection_->channel());
            connection_.reset();
        }

        if (peer_fd_ >= 0) {
            ::close(peer_fd_);
        }

        loop_.reset();//没有这个API？

        if (epoll_fd_ >= 0) {
            ::close(epoll_fd_);
        }
    }

     void dispatchRead() {
        connection_->channel().setRevents(EPOLLIN);
        connection_->channel().handleEvent();
    }

    protected:
    int epoll_fd_ = -1;
    int peer_fd_ = -1;

    std::unique_ptr<Eventloop> loop_;
    std::unique_ptr<Connection> connection_;
    std::vector<std::string> messages_;

};


TEST_F(ConnectionTest, CompleteLineProducesOneMessage) {
    ASSERT_EQ(::send(peer_fd_, "hello\n", 6, MSG_NOSIGNAL), 6);

    dispatchRead();

    ASSERT_EQ(messages_.size(), 1u);
    EXPECT_EQ(messages_[0], "hello");
}

TEST_F(ConnectionTest, IncompleteLineWaitsForRemainingBytes) {
    ASSERT_EQ(::send(peer_fd_, "hel", 3, MSG_NOSIGNAL), 3);

    dispatchRead();

    EXPECT_TRUE(messages_.empty());

    ASSERT_EQ(::send(peer_fd_, "lo\n", 3, MSG_NOSIGNAL), 3);

    dispatchRead();

    ASSERT_EQ(messages_.size(), 1u);
    EXPECT_EQ(messages_[0], "hello");
}

TEST_F(ConnectionTest, DeliversCompleteLinesAndKeepsPartialLine) {
    ASSERT_EQ(::send(peer_fd_, "a\nb\npar", 7, MSG_NOSIGNAL), 7);

    dispatchRead();

    EXPECT_EQ(messages_, (std::vector<std::string>{"a", "b"}));

    ASSERT_EQ(::send(peer_fd_, "t\n", 2, MSG_NOSIGNAL), 2);

    dispatchRead();

    EXPECT_EQ(
        messages_,
        (std::vector<std::string>{"a", "b", "part"})
    );
}

TEST_F(ConnectionTest, ClosingCallbackStopsMessages) {
    int close_calls = 0;

    connection_->setCloseCallback(
        [&](Connection&) {
            ++close_calls;
        }
    );

    connection_->setMessageCallback(
        [&](Connection& connection, std::string_view message) {
            messages_.emplace_back(message);
            connection.requestClose();
        }
    );

    ASSERT_EQ(::send(peer_fd_, "a\nb\n", 4, MSG_NOSIGNAL), 4);

    dispatchRead();

    EXPECT_EQ(close_calls, 1);
    EXPECT_EQ(messages_, (std::vector<std::string>{"a"}));
}

TEST_F(ConnectionTest, ClosingCallbackStopsMessages2) {
    int close_calls = 0;

    connection_->setCloseCallback(
        [&](Connection&) {
            ++close_calls;
        }
    );

    connection_->setMessageCallback(
        [&](Connection& connection, std::string_view message) {
            messages_.emplace_back(message);
            connection.requestClose();
        }
    );

    ASSERT_EQ(::send(peer_fd_, "a\nb\n", 4, MSG_NOSIGNAL), 4);

    dispatchRead();

    EXPECT_TRUE(connection_->is_WillDelete());
    EXPECT_EQ(close_calls, 1);
    EXPECT_EQ(messages_, (std::vector<std::string>{"a"}));
}
TEST_F(ConnectionTest, ClosingCallbackStopsMessages3) {
    int close_calls = 0;

    connection_->setCloseCallback(
        [&](Connection&) {
            ++close_calls;
        }
    );

    connection_->setMessageCallback(
        [&](Connection& connection, std::string_view message) {
            messages_.emplace_back(message);
            connection.requestClose();
        }
    );

    ASSERT_EQ(::send(peer_fd_, "a\nb\n", 4, MSG_NOSIGNAL), 4);

    dispatchRead();

    EXPECT_TRUE(connection_->is_WillDelete());
    EXPECT_EQ(close_calls, 1);
    EXPECT_EQ(messages_, (std::vector<std::string>{"a"}));
}

TEST_F(ConnectionTest, ClosingCallbackStopsMessages4) {
    int close_calls = 0;

   connection_->setCloseCallback(
    [&](Connection& connection) {
        ++close_calls;

        connection.channel().quit();

        loop_->queueTask([this] {
            connection_.reset();
        });
    }
);

    connection_->setMessageCallback(
        [&](Connection& connection, std::string_view message) {
            messages_.emplace_back(message);
            connection.requestClose();
        }
    );

    ASSERT_EQ(::send(peer_fd_, "a\nb\n", 4, MSG_NOSIGNAL), 4);

    dispatchRead();
    /*setRevents(EPOLLIN)：手动告诉 Channel，“这次收到的是可读事件”。
    handleEvent()：让 Channel 分发这个事件，调用之前注册的读回调。*/

    // 回调已经返回，但延迟任务还没有执行。
    ASSERT_NE(connection_.get(), nullptr);
    EXPECT_TRUE(connection_->is_WillDelete());
    EXPECT_EQ(close_calls, 1);
    EXPECT_EQ(messages_, (std::vector<std::string>{"a"}));

    // 执行延迟删除任务。
    loop_->runPendingTasks();
    EXPECT_EQ(connection_.get(), nullptr);
}