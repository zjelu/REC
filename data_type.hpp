#pragma once
#include <string>

enum class ReadResult {
    Ready,
    PeerClosed,
    Error
};

enum class FlushResult {
    WouldBlock,
    Done,
    Error
};
