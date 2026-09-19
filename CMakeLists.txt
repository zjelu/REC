cmake_minimum_required(VERSION 3.16)
project(ReactorTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(reactor
    Channel.cpp
    Poller.cpp
    Eventloop.cpp
    Tcpserver.cpp
    Acceptor.cpp
    Connection.cpp
)

target_include_directories(reactor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_executable(reactor_test
    main.cpp
)

add_executable(reactor_test2
    main2.cpp
)
target_link_libraries(reactor_test PRIVATE reactor)
target_link_libraries(
    reactor_test2
    PRIVATE reactor
)