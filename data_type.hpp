#pragma once
#include <string>

enum class ReadResult {
    Ready,
    PeerClosed,
    Error,
    OutOfSize
};

enum class FlushResult {
    WouldBlock,
    Done,
    Error
};
