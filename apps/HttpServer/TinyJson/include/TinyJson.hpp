#pragma once
#include <cstring>
#include <iostream>
#include <memory>
#include <queue>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>

class TinyJson {
public:
  enum ParseResult {
    OK,
    EXPECT_VALUE,
    INVALID_VALUE,
    ROOT_NOT_SINGULAR,
    NUMBER_TOO_BIG,
    INVALID_CHARACTER_ESCAPE, // 无效字符转义
    INVALID_CHARACTER,        // [0x0, 0x20) 无效字符
    QUOTATION_MISMATCH,       // 字符串中的"不匹配
    INVALID_UNICODE_HEX,
    INVALID_UNICODE_SURROGATE,
    ARRAY_MISS_COMMA_OR_SQUARE_BRACKET, // 数组缺少,或者缺少]
    OBJECT_MISS_COMMA_OR_CURLY_BRACKET, // object缺少,或者}
    OBJECT_MISS_KEY,                    // object缺少key
    OBJECT_MISS_COLON                   // 缺少:
  };

  enum JsonType {
    JSON_NULL,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
  };

  struct KVnode;

  struct JsonValue;
  struct Node {
    Node() : type{JsonType::JSON_NULL} {}
    Node(const Node &) = delete;
    Node &operator=(const Node &) = delete;
    Node &operator=(Node &&rhs) {
      switch (rhs.type) {
      case JsonType::JSON_NULL:
        this->type = JsonType::JSON_NULL;
        break;
      case JsonType::JSON_TRUE:
        this->type = JsonType::JSON_TRUE;
        break;
      case JsonType::JSON_FALSE:
        this->type = JsonType::JSON_FALSE;
        break;
      case JsonType::JSON_NUMBER:
        this->type = JsonType::JSON_NUMBER;
        this->number = rhs.number;
        break;
      case JsonType::JSON_STRING:
        this->type = JsonType::JSON_STRING;
        this->str = rhs.str;
        this->strLen = rhs.strLen;
        rhs.type = JsonType::JSON_NULL;
        rhs.str = nullptr;
        rhs.strLen = 0;
        break;
      case JsonType::JSON_ARRAY:
        this->type = JsonType::JSON_ARRAY;
        this->arr = rhs.arr;
        this->arrLen = rhs.arrLen;
        rhs.type = JsonType::JSON_NULL;
        rhs.arr = nullptr;
        rhs.arrLen = 0;
        break;
      case JsonType::JSON_OBJECT:
        this->type = JsonType::JSON_OBJECT;
        this->object = rhs.object;
        this->objSize = rhs.objSize;
        rhs.type = JsonType::JSON_NULL;
        rhs.object = nullptr;
        rhs.objSize = 0;
        break;
      }
      return *this;
    }
    Node(Node &&rhs) {
      switch (rhs.type) {
      case JsonType::JSON_NULL:
        this->type = JsonType::JSON_NULL;
        break;
      case JsonType::JSON_TRUE:
        this->type = JsonType::JSON_TRUE;
        break;
      case JsonType::JSON_FALSE:
        this->type = JsonType::JSON_FALSE;
        break;
      case JsonType::JSON_NUMBER:
        this->type = JsonType::JSON_NUMBER;
        this->number = rhs.number;
        break;
      case JsonType::JSON_STRING:
        this->type = JsonType::JSON_STRING;
        this->str = rhs.str;
        this->strLen = rhs.strLen;
        rhs.type = JsonType::JSON_NULL;
        rhs.str = nullptr;
        rhs.strLen = 0;
        break;
      case JsonType::JSON_ARRAY:
        this->type = JsonType::JSON_ARRAY;
        this->arr = rhs.arr;
        this->arrLen = rhs.arrLen;
        rhs.type = JsonType::JSON_NULL;
        rhs.arr = nullptr;
        rhs.arrLen = 0;
        break;
      case JsonType::JSON_OBJECT:
        this->type = JsonType::JSON_OBJECT;
        this->object = rhs.object;
        this->objSize = rhs.objSize;
        rhs.type = JsonType::JSON_NULL;
        rhs.object = nullptr;
        rhs.objSize = 0;
        break;
      }
    }
    JsonType type;
    union {
      double number;
      struct {
        char *str;
        size_t strLen; // for string
      };
      struct {
        Node **arr;
        size_t arrLen; // for array
      };

      struct {
        KVnode *object;
        size_t objSize; // for object
      };
    };

    void setStr(const char *s, size_t sLen) {
      if (sLen == 0) [[unlikely]] {
        type = JsonType::JSON_STRING;
        str = nullptr;
        strLen = 0;
      } else {
        char *ret = static_cast<char *>(::malloc(sLen));
        if (ret == nullptr) [[unlikely]] {
          std::abort();
        }
        type = JsonType::JSON_STRING;
        str = ret;
        strLen = sLen;
        ::memcpy(str, s, sLen);
      }
    }
    std::string_view getStr() { return std::string_view(str, strLen); }
  };

  struct KVnode {
    char *str;
    size_t strLen; // for key
    Node *v;
    KVnode() : str{nullptr}, strLen{0}, v{} {}
  };

  struct JsonValue {
    Node *root;
    JsonValue() : root{static_cast<Node *>(::malloc(sizeof(Node)))} {}
    JsonValue(Node *v) : root{v} {}
    JsonValue(const JsonValue &) = delete;
    JsonValue(JsonValue &&) = delete;
    JsonValue &operator=(const JsonValue &) = delete;
    JsonValue &operator=(JsonValue &&) = delete;
    ~JsonValue() {
      if (root == nullptr)
        return;
      std::queue<Node *> que;
      que.push(root);
      root = nullptr;
      while (!que.empty()) {
        Node *tmp = que.front();
        que.pop();
        switch (tmp->type) {
        case JsonType::JSON_STRING:
          ::free(tmp->str);
          ::free(tmp);
          break;
        case JsonType::JSON_ARRAY:
          for (size_t i = 0; i < tmp->arrLen; ++i) {
            que.push(tmp->arr[i]);
          }
          ::free(tmp->arr);
          ::free(tmp);
          break;
        case JsonType::JSON_OBJECT:
          for (size_t i = 0; i < tmp->objSize; ++i) {
            ::free(tmp->object[i].str);
            que.push(tmp->object[i].v);
          }
          ::free(tmp->object);
          ::free(tmp);
          break;
        default:
          ::free(tmp);
          break;
        }
      }
    }
  };

  ParseResult parse(JsonValue *);

  TinyJson() {}

  TinyJson(std::string_view sv) : visitedPos(sv) {
    inStack = std::make_unique<InternalStack>();
  }

  TinyJson(const TinyJson &) = delete;

  TinyJson &operator=(const TinyJson &) = delete;

  TinyJson &operator=(TinyJson &&rhs) {
    visitedPos = rhs.visitedPos;
    inStack = std::move(rhs.inStack);
    return *this;
  }

  std::span<char> stringify(JsonValue *);

  std::string stringify2(JsonValue *);

private:
  // 记录状态
  std::string_view visitedPos;

  struct InternalStack {
    // https://stackoverflow.com/questions/11781724/do-i-really-have-to-worry-about-alignment-when-using-placement-new-operator
    char *ptr;
    size_t size, top;

    inline static std::size_t STACK_INIT_SIZE = 256;
    void *push(size_t);
    void *pop(size_t);
    void reset() { top = 0; }
    InternalStack()
        : ptr{(char *)malloc(STACK_INIT_SIZE)}, size{STACK_INIT_SIZE}, top{} {}
    ~InternalStack() {
      if (ptr != nullptr) {
        free(ptr);
      }
    }
  };
  std::unique_ptr<InternalStack> inStack;
  void parse_whitespace();
  ParseResult parse_value(Node *);
  ParseResult parse_literal(Node *, std::string_view, JsonType);
  ParseResult parse_number(Node *);
  ParseResult parse_string(Node *);
  ParseResult parse_string_raw(char *&, std::size_t &);
  bool parse_hex4(unsigned *);
  void encode_utf8(unsigned);
  ParseResult parse_array(Node *);
  ParseResult parse_object(Node *);

  // stringify
  void stringify_value(InternalStack &, Node *);
  void stringify_string(InternalStack &, std::string_view);

  void stringify_value(std::string &, Node *);
  void stringify_string(std::string &, std::string_view);
};
