#include <functional>
#include <iostream>
#include <utility>

class Notifier {
public:
    using Callback =
        std::function<void(int)>;

    void setCallback(Callback callback) {
        callback_ = std::move(callback);
    }

    void notify(int value) {
        if (callback_) {
            callback_(value);
        }
    }

private:
    Callback callback_;
};

class Receiver {
public:
    void receive(int value) {
        std::cout
            << "received: "
            << value
            << '\n';
    }
};

int main() {
    Notifier notifier;
    Receiver receiver;

    notifier.setCallback(
        [&receiver](int value) {
            receiver.receive(value);
        }
    );

    notifier.notify(42);
}