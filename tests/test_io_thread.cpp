#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <iostream>
#include <logger.hpp>
#include <tcp/io_thread.hpp>
#include <thread>

TEST_CASE("Test io_thread", "[io_thread]") {
    LInfo("TEST_CASE io_thread");
    TinyNet::IOThreadPool pool(2);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(20s);
}
