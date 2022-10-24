#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <tcp/tcp_buffer.hpp>

TEST_CASE("Test tcp_buffer_init", "[init]") {
  TinyNet::TcpBuffer buffer(4);
  REQUIRE(buffer.getSize() == 4);
  REQUIRE(buffer.readIndex() == 0);
  REQUIRE(buffer.writeIndex() == 0);
  REQUIRE(buffer.writeAble() == 4);
  REQUIRE(buffer.readAble() == 0);
}

TEST_CASE("Test tcp_buffer_writeToBuffer", "[write_to_buffer & adjustBuffer]") {
  TinyNet::TcpBuffer buffer(4);
  REQUIRE(buffer.getSize() == 4);
  REQUIRE(buffer.readIndex() == 0);
  REQUIRE(buffer.writeIndex() == 0);
  REQUIRE(buffer.writeAble() == 4);
  REQUIRE(buffer.readAble() == 0);

  buffer.writeToBuffer("abc", 3);
  REQUIRE(buffer.getSize() == 4);
  REQUIRE(buffer.readIndex() == 0);
  REQUIRE(buffer.writeIndex() == 3);
  REQUIRE(buffer.writeAble() == 1);
  REQUIRE(buffer.readAble() == 3);

  std::vector<char> tmp;
  buffer.readFromBuffer(tmp, 2);
  REQUIRE(buffer.getSize() == 4);
  REQUIRE(buffer.readIndex() == 0);
  REQUIRE(buffer.writeIndex() == 1);
  REQUIRE(buffer.writeAble() == 3);
  REQUIRE(buffer.readAble() == 1);
  REQUIRE(tmp.size() == 2);
  REQUIRE(tmp[0] == 'a');
  REQUIRE(tmp[1] == 'b');

  tmp.clear();
  buffer.readFromBuffer(tmp, 1);
  REQUIRE(buffer.getSize() == 4);
  REQUIRE(buffer.readIndex() == 1);
  REQUIRE(buffer.writeIndex() == 1);
  REQUIRE(buffer.writeAble() == 3);
  REQUIRE(buffer.readAble() == 0);
  REQUIRE(tmp.size() == 1);

  // 扩容
  buffer.writeToBuffer("abcdefghikl", 12);
  REQUIRE(buffer.readIndex() == 0);
  REQUIRE(buffer.writeIndex() == 12);
  REQUIRE(buffer.writeAble() == 7);
  REQUIRE(buffer.getSize() == 19);
}