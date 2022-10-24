#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <iostream>
#include <queue>

class MyClass {
  MyClass() { std::cout << "MyClass()\n"; }
  ~MyClass() { std::cout << "~MyClass()\n"; }
};

TEST_CASE("Test queue", "[test_queue]") {}