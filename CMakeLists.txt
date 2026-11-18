cmake_minimum_required(VERSION 3.16)
project(ReactorTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

option(REACTOR_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

# 为自己的目标统一配置警告和运行时检查。
function(configure_reactor_target target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )

        if(REACTOR_ENABLE_SANITIZERS)
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )

            target_link_options(${target} PUBLIC
                -fsanitize=address,undefined
            )
        endif()
    endif()
endfunction()

# 公共 Reactor 实现。
add_library(reactor STATIC
    Channel.cpp
    Poller.cpp
    Eventloop.cpp
    Acceptor.cpp
    Connection.cpp
)

target_include_directories(reactor PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(reactor PUBLIC Threads::Threads)
configure_reactor_target(reactor)

# 原单线程 Server。
add_library(reactor_server_single STATIC
    Tcpserver.cc
)

target_link_libraries(reactor_server_single PUBLIC reactor)
configure_reactor_target(reactor_server_single)

# 新多线程 Server。暂时不需要新版 main。
add_library(reactor_server_thread STATIC
    Tcpserver_thread.cc
    worker.cc
)

target_link_libraries(reactor_server_thread PUBLIC reactor)
configure_reactor_target(reactor_server_thread)

# 多线程服务入口
add_executable(reactor_test2 main2.cpp)

target_link_libraries(reactor_test2 PRIVATE
    reactor_server_thread
)

configure_reactor_target(reactor_test2)

# GoogleTest。
enable_testing()

set(GOOGLETEST_VERSION 1.17.0)

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../independent_project/googletest"
    "${CMAKE_CURRENT_BINARY_DIR}/googletest"
)

add_executable(reactor_unit_tests
    tests/Channel_test.cpp
    tests/Eventloop_test.cpp
    tests/Connection_test.cpp
)

target_link_libraries(reactor_unit_tests PRIVATE
    reactor
    GTest::gtest_main
)

configure_reactor_target(reactor_unit_tests)

include(GoogleTest)
gtest_discover_tests(reactor_unit_tests)