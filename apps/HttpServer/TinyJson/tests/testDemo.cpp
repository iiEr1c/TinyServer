#define CATCH_CONFIG_MAIN
#include <TinyJson.hpp>
#include <array>
#include <catch2/catch.hpp>
#include <string>

#include <fcntl.h>
#include <sys/stat.h>

TEST_CASE("Test parse_space", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val{};
  TinyJson json = " \t\n\r"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::EXPECT_VALUE);

  json = "\t false\r"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_FALSE);

  json = "\r\tfalse\r\t"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_FALSE);

  json = "\r\tfalse \r\t\ne"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::ROOT_NOT_SINGULAR);
}

TEST_CASE("Test parse_null", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json = ""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::EXPECT_VALUE);

  json = "null"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_NULL);

  json = "null "sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_NULL);

  json = "nu11"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::INVALID_VALUE);

  json = "null l"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::ROOT_NOT_SINGULAR);
}

TEST_CASE("Test parse_true", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json = ""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::EXPECT_VALUE);

  json = "true"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_TRUE);

  json = " true "sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_TRUE);

  json = "t^ue"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::INVALID_VALUE);

  json = "truee"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::ROOT_NOT_SINGULAR);
}

TEST_CASE("Test parse_false", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json = ""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::EXPECT_VALUE);

  json = "false"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_FALSE);

  json = "  false  "sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_FALSE);

  json = "f4lse"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::INVALID_VALUE);

  json = "false e"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::ROOT_NOT_SINGULAR);
}

TEST_CASE("Test number", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json = ""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::EXPECT_VALUE);

  json = "123.45"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_NUMBER);
  REQUIRE(val.root->number == 123.45);

  json = "123.45 "sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_NUMBER);
  REQUIRE(val.root->number == 123.45);

  json = "  0.12  "sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->type == TinyJson::JsonType::JSON_NUMBER);
  REQUIRE(val.root->number == 0.12);
}

TEST_CASE("Test string", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json = "\"\""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->getStr().size() == 0);

  json = "\"abcd\""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->getStr().size() == 4);

  json = "\""sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::QUOTATION_MISMATCH);

  json = "\"ab\\"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::INVALID_CHARACTER_ESCAPE);

  json = "\"\0x0\0x1"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::INVALID_CHARACTER);

  REQUIRE(val.root->type == TinyJson::JSON_STRING);
  val.~JsonValue();
  val.root = (TinyJson::Node *)::malloc(sizeof(TinyJson::Node));
  json = "\"\u4f60\u597dabcdefghijk\""sv; // unicode为你好
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->getStr() == "你好abcdefghijk"sv);
}

TEST_CASE("Test array", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json =
      "[[\"abcdef\",2,3], [4,\"edfddjlflfl\",6], [[1,2,3], [4,5,6]]  ]"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->arrLen == 3);
  REQUIRE(val.root->arr[0]->arrLen == 3);
  REQUIRE(val.root->arr[0]->arrLen == 3);
  REQUIRE(val.root->arr[0]->arr[0]->getStr() == "abcdef"sv);
  REQUIRE(val.root->arr[2]->arrLen == 2);
  REQUIRE(val.root->arr[2]->arr[0]->arrLen == 3);
  REQUIRE(val.root->arr[2]->arr[0]->arr[0]->number == 1);
  REQUIRE(val.root->arr[2]->arr[0]->arr[1]->number == 2);
  REQUIRE(val.root->arr[2]->arr[0]->arr[2]->number == 3);
}

TEST_CASE("Test null object", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json = "{}"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->objSize == 0);
}

TEST_CASE("Test object", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  TinyJson json =
      "{\"A\": 10, \"B\": {\"B1\": [1, 2, 3, {\"C\": \"abcdef\"}]}}"sv;
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
}

TEST_CASE("Test stringify", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  std::string_view input{"\
  {\
    \"code\" : 1, \"msg\" : \"success\", \"data\" : {\
      \"name\" : \"pig\", \"age\" : \"18\", \"sex\" : \"man\", \"hometown\" : {\
        \"province\" : \"江西省\", \"city\" : \"抚州市\", \"county\" : \"崇仁县\"\
      }\
    }\
  }"};
  TinyJson json(input);
  REQUIRE(json.parse(&val) == TinyJson::ParseResult::OK);
  REQUIRE(val.root->objSize == 3);
  std::string_view sv(val.root->object[2].v->object[3].v->object[0].str,
                      val.root->object[2].v->object[3].v->object[0].strLen);
  REQUIRE(sv == "province"sv);
  sv = std::string_view(val.root->object[2].v->object[3].v->object[1].str,
                        val.root->object[2].v->object[3].v->object[1].strLen);
  REQUIRE(sv == "city"sv);
  sv = std::string_view(val.root->object[2].v->object[3].v->object[2].str,
                        val.root->object[2].v->object[3].v->object[2].strLen);
  REQUIRE(sv == "county"sv);

  sv =
      std::string_view(val.root->object[2].v->object[3].v->object[0].v->str,
                       val.root->object[2].v->object[3].v->object[0].v->strLen);
  REQUIRE(sv == "江西省"sv);

  sv =
      std::string_view(val.root->object[2].v->object[3].v->object[1].v->str,
                       val.root->object[2].v->object[3].v->object[1].v->strLen);
  REQUIRE(sv == "抚州市"sv);

  sv =
      std::string_view(val.root->object[2].v->object[3].v->object[2].v->str,
                       val.root->object[2].v->object[3].v->object[2].v->strLen);
  REQUIRE(sv == "崇仁县"sv);

  std::span<char> sp = json.stringify(&val);
  auto str = json.stringify2(&val);
  REQUIRE(std::string_view(sp.begin(), sp.end()) == std::string_view(str));
  ::free(sp.data());
}

TEST_CASE("Test stringify2", "[parse]") {
  using namespace std::literals;
  TinyJson::JsonValue val;
  std::string_view input(
      "{\"code\":1,\"msg\":\"success\",\"data\":{\"name\":\"pig\",\"age\":"
      "\"18\",\"sex\":\"man\",\"hometown\":{\"province\":\"江西省\",\"city\":"
      "\"抚州市\",\"county\":\"崇仁县\"}}}");
  TinyJson json(input);
  json.parse(&val);
  auto str = json.stringify2(&val);
  REQUIRE(input == std::string_view(str));
}