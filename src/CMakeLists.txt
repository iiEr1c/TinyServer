set(HEADER_LIST
    "logger.hpp"
    "epollfd.hpp"
    "FdEvent.hpp"
    "reactor.hpp"
    "timer.hpp"
    "slot.hpp"
    "spsc.hpp"
    "object_pool.hpp"
    "threadUnsafeSharedPtr.hpp"
    "tcp/ip_v4_address.hpp"
    "tcp/tcp_buffer.hpp"
    "tcp/tcp_connection.hpp"
    "tcp/tcp_connection_wheel.hpp"
    "tcp/acceptor.hpp"
    "tcp/io_thread.hpp"
    "tcp/io_thread_pool.hpp"
    "tcp/tcp_server.hpp")
set(net_SRCS
    "FdEvent.cpp"
    "reactor.cpp"
    "timer.cpp"
    "tcp/ip_v4_address.cpp"
    "tcp/tcp_buffer.cpp"
    "tcp/tcp_connection.cpp"
    "tcp/tcp_connection_wheel.cpp"
    "tcp/acceptor.cpp"
    "tcp/io_thread.cpp"
    "tcp/io_thread_pool.cpp"
    "tcp/tcp_server.cpp")

add_library(TinyNet ${net_SRCS} ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(TinyNet PUBLIC ./)

# All users of this library will need at least C++11
target_compile_features(TinyNet PUBLIC cxx_std_20)

target_compile_options(TinyNet PRIVATE
            ${CXX_FLAGS}
            "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
            "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>")

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(TinyNet PRIVATE ${SANITIZE_FLAG})
endif()
#target_compile_options(TinyNet PRIVATE "-fsanitize=address" "-fsanitize=undefined")
target_link_libraries(TinyNet PRIVATE spdlog::spdlog)