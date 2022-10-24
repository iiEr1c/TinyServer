#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <tcp/ip_v4_address.hpp>

TEST_CASE("Test ipv4", "[ipv4]") {
    TinyNet::IPv4Address server("127.0.0.1", 8080);
}