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

# 启用 CTest
enable_testing()

# 引入已有的 GoogleTest 源码
# 本地这份 GoogleTest 的 CMakeLists.txt 需要这个版本变量
set(GOOGLETEST_VERSION 1.17.0)

add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../independent_project/googletest"
    "${CMAKE_CURRENT_BINARY_DIR}/googletest"
)

# 测试程序：需要先创建这个源文件
add_executable(reactor_unit_tests
    tests/Channel_test.cpp
    tests/Eventloop_test.cpp
    tests/Connection_test.cpp
)

target_link_libraries(reactor_unit_tests PRIVATE
    reactor
    GTest::gtest_main
)

# 自动将 TEST / TEST_F 注册到 CTest
include(GoogleTest)
gtest_discover_tests(reactor_unit_tests)

#开启编译警告
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target reactor reactor_unit_tests)
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
        )
    endforeach()
endif()

#开启 ASan 和 UBSan。 它们在运行测试时检查内存错误和部分未定义行为
option(REACTOR_ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

if(REACTOR_ENABLE_SANITIZERS
   AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(reactor PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )

    # 让链接 reactor 的可执行文件带上 sanitizer 运行库
    target_link_options(reactor PUBLIC
        -fsanitize=address,undefined
    )

    target_compile_options(reactor_unit_tests PRIVATE
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
    )
endif()