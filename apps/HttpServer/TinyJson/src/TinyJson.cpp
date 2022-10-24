#include <TinyJson.hpp>
#include <algorithm>
#include <cassert>
#include <charconv>
#include <cmath>
#include <cstring>
#include <iostream>

void *TinyJson::InternalStack::push(size_t len) {
  assert(len > 0);
  if (top + len > size) {
    while (top + len >= size) {
      size += size >> 1;
    }
    ptr = static_cast<char *>(realloc(ptr, size));
  }
  void *ret = ptr + top;
  top += len;
  return ret;
}

void *TinyJson::InternalStack::pop(size_t len) {
  assert(top >= len);
  top -= len;
  return ptr + top;
}

TinyJson::ParseResult TinyJson::parse(JsonValue *v) {
  assert(v != nullptr);
  parse_whitespace();
  TinyJson::ParseResult ret;
  if ((ret = parse_value(v->root)) == TinyJson::ParseResult::OK) {
    parse_whitespace();
    if (visitedPos.size() > 0) {
      return ParseResult::ROOT_NOT_SINGULAR;
    }
  }
  return ret;
}

TinyJson::ParseResult TinyJson::parse_value(Node *v) {
  using namespace std::literals;
  if (visitedPos.size() == 0) {
    return TinyJson::ParseResult::EXPECT_VALUE;
  }
  switch (visitedPos[0]) {
  case 'n':
    return parse_literal(v, "null"sv, JSON_NULL);
  case 't':
    return parse_literal(v, "true"sv, JSON_TRUE);
  case 'f':
    return parse_literal(v, "false"sv, JSON_FALSE);
  case '\"':
    return parse_string(v);
  case '[':
    return parse_array(v);
  case '{':
    return parse_object(v);
  default:
    return parse_number(v);
  }
}

void TinyJson::parse_whitespace() {
  std::string_view &sv = visitedPos;
  sv.remove_prefix(std::min(sv.find_first_not_of(" \t\n\r"), sv.size()));
}

TinyJson::ParseResult TinyJson::parse_literal(TinyJson::Node *v,
                                              std::string_view literal,
                                              TinyJson::JsonType type) {
  std::string_view &sv = visitedPos;
  assert(sv.size() >= 1);
  if (sv.starts_with(literal)) [[likely]] {
    sv.remove_prefix(literal.size());
    v->type = type;
    return TinyJson::ParseResult::OK;
  } else {
    return TinyJson::ParseResult::INVALID_VALUE;
  }
}

TinyJson::ParseResult TinyJson::parse_number(TinyJson::Node *v) {
  std::string_view &sv = visitedPos;
  size_t len = sv.length();
  size_t i = 0;
  if (i < len && sv[i] == '-')
    ++i;
  if (i < len && sv[i] == '0')
    ++i;
  else {
    if (i >= len || !(sv[i] >= '1' && sv[i] <= '9')) {
      return TinyJson::ParseResult::INVALID_VALUE;
    }
    for (++i; i < len && std::isdigit(sv[i]); ++i)
      ;
  }
  if (i < len && sv[i] == '.') {
    ++i;
    if (i >= len || !std::isdigit(sv[i]))
      return TinyJson::ParseResult::INVALID_VALUE;
    for (++i; i < len && std::isdigit(sv[i]); ++i)
      ;
  }
  if (i < len && (sv[i] == 'E' || sv[i] == 'e')) {
    ++i;
    if (i < len && (sv[i] == '+' || sv[i] == '-'))
      ++i;
    if (i >= len || !std::isdigit(sv[i]))
      return TinyJson::ParseResult::INVALID_VALUE;
    for (++i; i < len && std::isdigit(sv[i]); ++i)
      ;
  }

  v->number = strtod(sv.data(), nullptr);
  if (errno == ERANGE && (v->number == HUGE_VAL || v->number == -HUGE_VAL))
    return TinyJson::ParseResult::NUMBER_TOO_BIG;
  v->type = JsonType::JSON_NUMBER;
  sv.remove_prefix(i);
  return ParseResult::OK;
}

TinyJson::ParseResult TinyJson::parse_string_raw(char *&str, std::size_t &len) {
  std::string_view &sv = visitedPos;
  assert(sv.front() == '\"');
  size_t head = inStack->top;
  std::size_t svLen = sv.size(), i = 0, strLen;
  unsigned u1, u2;
  for (++i; i < svLen;) {
    char ch = sv[i++];
    switch (ch) {
    case '\"':
      strLen = inStack->top - head;
      str = static_cast<char *>(inStack->pop(strLen));
      len = strLen;
      sv.remove_prefix(i);
      return ParseResult::OK;
    case '\\':
      if (i == svLen) {
        inStack->top = head;
        return ParseResult::INVALID_CHARACTER_ESCAPE;
      }
      switch (sv[i++]) {
      case '\"':
        *static_cast<char *>(inStack->push(1)) = '\"';
        break;
      case '\\':
        *static_cast<char *>(inStack->push(1)) = '\\';
        break;
      case '/':
        *static_cast<char *>(inStack->push(1)) = '/';
        break;
      case 'b':
        *static_cast<char *>(inStack->push(1)) = '\b';
        break;
      case 'f':
        *static_cast<char *>(inStack->push(1)) = '\f';
        break;
      case 'n':
        *static_cast<char *>(inStack->push(1)) = '\n';
        break;
      case 'r':
        *static_cast<char *>(inStack->push(1)) = '\r';
        break;
      case 't':
        *static_cast<char *>(inStack->push(1)) = '\t';
        break;
      case 'u':
        // 解析Unicode
        if (!parse_hex4(&u1))
          return ParseResult::INVALID_UNICODE_HEX;
        if (u1 >= 0xD800 && u1 <= 0xDBFF) {
          if (sv.size() < 2 || sv[0] != '\\' || sv[1] != 'u')
            return ParseResult::INVALID_UNICODE_SURROGATE; // 必须以\u开头
          sv.remove_prefix(2);
          if (!parse_hex4(&u2))
            return ParseResult::INVALID_UNICODE_HEX;
          if (u2 < 0xDC00 || u2 > 0xDFFF)
            return ParseResult::INVALID_UNICODE_SURROGATE;
          u1 = (((u1 - 0xD800) << 10) | (u2 - 0xDC00)) + 0x10000;
        }
        // 对解析出来的Unicode进行编码
        encode_utf8(u1);
        break;
      default:
        inStack->top = head;
        return ParseResult::INVALID_CHARACTER_ESCAPE;
      }
      break;
    default:
      if (static_cast<unsigned char>(ch) < 0x20) {
        inStack->top = head;
        return ParseResult::INVALID_CHARACTER;
      }
      // 处理普通字符
      *static_cast<char *>(inStack->push(1)) = ch;
    }
  }
  // 现在需要解决的的是如果在svLen内没有解析到\"的情况
  // 如果成功解析, 那么此时的sv[i]的前一个字符一定是",
  // 因此我们可以利用这一点来判断 考虑 "然后结束和"A然后结束
  // 这里要么i==1要么不等于\",没有其他的情况了.
  // if (i == 1 || sv[i - 1] != '\"') {
  //     return ParseResult::QUOTATION_MISMATCH;
  // }
  return ParseResult::QUOTATION_MISMATCH;
}

TinyJson::ParseResult TinyJson::parse_string(TinyJson::Node *v) {
  char *str = nullptr;
  std::size_t len = 0;
  auto ret = parse_string_raw(str, len);
  if (ret == TinyJson::ParseResult::OK) {
    v->setStr(str, len);
  }
  return ret;
}

// 读入unicode编码
bool TinyJson::parse_hex4(unsigned *u) {
  std::string_view &sv = visitedPos;
  if (sv.size() < 4)
    return false; // 至少4个byte
  *u = 0;
  // 从高位解析到低位, 顺序解析
  for (int i = 0; i < 4; ++i) {
    char ch = sv[i];
    *u <<= 4;
    if (ch >= '0' && ch <= '9')
      *u |= ch - '0';
    else if (ch >= 'A' && ch <= 'F')
      *u |= ch - ('A' - 10);
    else if (ch >= 'a' && ch <= 'f')
      *u |= ch - ('a' - 10); // 16进制支持大小写
    else
      return false;
  }
  visitedPos.remove_prefix(4); // 解析成功
  return true;
}

// 将unicode码转换为utf-8编码并储存
// https://www.herongyang.com/Unicode/UTF-8-UTF-8-Encoding-Algorithm.html
void TinyJson::encode_utf8(unsigned u) {
  if (u <= 0x7F) {
    *static_cast<char *>(inStack->push(1)) =
        static_cast<unsigned char>(u & 0xFF);
  } else if (u <= 0x7FF) {
    char *start = static_cast<char *>(inStack->push(2));
    *start = static_cast<unsigned char>(0xC0 | ((u >> 6) & 0xFF));
    *(start + 1) = 0x80 | (u & 0x3F);
  } else if (u <= 0xFFFF) {
    char *start = static_cast<char *>(inStack->push(3));
    *start = static_cast<unsigned char>(0xE0 | ((u >> 12) & 0xFF));
    *(start + 1) = 0x80 | ((u >> 6) & 0x3F);
    *(start + 2) = 0x80 | (u & 0x3F);
  } else {
    char *start = static_cast<char *>(inStack->push(4));
    *start = static_cast<unsigned char>(0xF0 | ((u >> 18) & 0xFF));
    *(start + 1) = 0x80 | ((u >> 12) & 0x3F);
    *(start + 2) = 0x80 | ((u >> 6) & 0x3F);
    *(start + 3) = 0x80 | (u & 0x3F);
  }
}

TinyJson::ParseResult TinyJson::parse_array(Node *v) {
  std::string_view &sv = visitedPos;
  assert(sv.front() == '[');
  sv.remove_prefix(1); // 跳过开头的[
  parse_whitespace();  // 去除空格, [ a,   b,  c]
  if (sv.size() == 0) [[unlikely]] {
    // 如果数组到此结束, 说明是这样的情况: [
    return ParseResult::ARRAY_MISS_COMMA_OR_SQUARE_BRACKET;
  }
  if (sv.front() == ']') {
    sv.remove_prefix(1); // 到达数组末端, 结束
    v->type = JsonType::JSON_ARRAY;
    v->arr = nullptr;
    v->arrLen = 0;
    return ParseResult::OK;
  }

  ParseResult ret;
  size_t arrSize = 0; // 数组大小
  for (;;) {
    Node *val = static_cast<Node *>(::malloc(sizeof(Node))); // 分配内存
    if ((ret = parse_value(val)) != ParseResult::OK) {
      break;
    }
    // 这里我们使用memcpy将对象复制到stack中, placement new是未定义行为
    ::memcpy(inStack->push(sizeof(Node *)), &val,
             sizeof(Node *)); // 其实就是将Node*指针压栈而已
    ++arrSize;
    parse_whitespace();
    if (sv.size() == 0) {
      ret = ARRAY_MISS_COMMA_OR_SQUARE_BRACKET; // 至少是,或者]结束
      break;
    } else if (sv.front() == ',') {
      // 说明可以解析数组的下一个元素
      sv.remove_prefix(1);
      parse_whitespace();
    } else if (sv.front() == ']') {
      // 数组末端
      sv.remove_prefix(1);
      v->type = JsonType::JSON_ARRAY;
      v->arrLen = arrSize;
      arrSize *= sizeof(Node *); // 我们存储的是Node *, 并非Node
      v->arr = static_cast<Node **>(::malloc(arrSize)); // todo:数据对齐
      ::memcpy(v->arr, inStack->pop(arrSize), arrSize); // 将指针copy到v->arr中
      return ParseResult::OK;
    } else {
      ret = ParseResult::ARRAY_MISS_COMMA_OR_SQUARE_BRACKET;
      break;
    }
  }
  // 如果解析array失败, 将栈中的指针释放即可
  for (size_t i = 0; i < arrSize; ++i) {
    Node *src = static_cast<Node *>(
        inStack->pop(sizeof(Node))); // 这里将Node *强转为JsonValue了
    JsonValue val{src};
  }
  return ret;
}

TinyJson::ParseResult TinyJson::parse_object(Node *v) {
  std::string_view &sv = visitedPos;
  assert(sv.front() == '{');
  sv.remove_prefix(1);
  parse_whitespace();
  if (sv.size() == 0)
    return ParseResult::OBJECT_MISS_COMMA_OR_CURLY_BRACKET;
  if (sv.front() == '}') {
    sv.remove_prefix(1);
    v->type = JsonType::JSON_OBJECT;
    v->object = nullptr;
    v->objSize = 0;
    return ParseResult::OK;
  }

  size_t i = 0, objSizse = 0;
  KVnode kv{};
  ParseResult ret;
  for (;;) {
    if (sv.front() != '"') {
      ret = ParseResult::OBJECT_MISS_KEY;
      break;
    }
    char *stackAddr = nullptr;
    if ((ret = parse_string_raw(stackAddr, kv.strLen)) != ParseResult::OK)
      break;
    kv.str = static_cast<char *>(::malloc(kv.strLen));
    ::memcpy(kv.str, stackAddr, kv.strLen);
    parse_whitespace();
    if (sv.front() != ':') {
      // kv中间必须是:
      ret = ParseResult::OBJECT_MISS_COLON;
      break;
    }
    sv.remove_prefix(1);
    parse_whitespace();
    kv.v =
        static_cast<Node *>(::malloc(sizeof(Node))); // 给kv.v分配内存,未处理oom
    if ((ret = parse_value(kv.v)) != ParseResult::OK)
      break;
    // 将该kv对压入栈中
    ::memcpy(inStack->push(sizeof(KVnode)), &kv,
             sizeof(KVnode)); // 这里相当于move, 另外, kvnode并没有析构掉对象,
                              // 应该弄成支持std::move的栈
    ++objSizse;
    kv.str = nullptr;
    kv.strLen = 0;
    kv.v = nullptr;
    parse_whitespace();
    if (sv.size() == 0) {
      ret = ParseResult::OBJECT_MISS_COMMA_OR_CURLY_BRACKET;
      break;
    } else if (sv.front() == ',') {
      sv.remove_prefix(1); //跳过,
      parse_whitespace();
    } else if (sv.front() == '}') {
      // 只能是'}', 对象解析结束
      sv.remove_prefix(1);
      size_t memSize = objSizse * sizeof(KVnode);
      v->type = JSON_OBJECT;
      v->objSize = objSizse;
      v->object = static_cast<KVnode *>(::malloc(memSize));
      ::memcpy(v->object, inStack->pop(memSize), memSize);
      return ParseResult::OK;
    } else {
      // 其他字符
      ret = ParseResult::OBJECT_MISS_COMMA_OR_CURLY_BRACKET;
      break;
    }
  }

  // 释放暂时存储在栈中对象的内存
  for (size_t j = 0; j < objSizse; ++j) {
    // 这里同样使用和之前array一样的笨方法...
    // 如果inStack是对齐的, 那么可以直接在上边原地析构
    // 其实是ub行为, 因为字节可能不对齐
    KVnode *tmpkv = static_cast<KVnode *>(inStack->pop(sizeof(KVnode)));
    ::free(tmpkv->str);
    JsonValue val(tmpkv->v);
  }
  return ret;
}

void TinyJson::stringify_value(InternalStack &stack, Node *val) {
  size_t i = 0;
  switch (val->type) {
  case JsonType::JSON_NULL:
    ::memcpy(stack.push(4), "null", 4);
    break;
  case JsonType::JSON_TRUE:
    ::memcpy(stack.push(4), "true", 4);
    break;
  case JsonType::JSON_FALSE:
    ::memcpy(stack.push(5), "false", 5);
    break;
  case JsonType::JSON_NUMBER:
    stack.pop(32 - sprintf(static_cast<char *>(stack.push(32)), "%.17g",
                           val->number));
    break;
  case JsonType::JSON_STRING:
    stringify_string(stack, std::string_view{val->str, val->strLen});
    break;
  case JsonType::JSON_ARRAY:
    *static_cast<char *>(stack.push(1)) = '[';
    for (i = 0; i < val->arrLen; ++i) {
      if (i > 0) {
        *static_cast<char *>(stack.push(1)) = ',';
      }
      stringify_value(stack, val->arr[i]);
    }
    *static_cast<char *>(stack.push(1)) = ']';
    break;
  case JsonType::JSON_OBJECT:
    *static_cast<char *>(stack.push(1)) = '{';
    for (i = 0; i < val->objSize; ++i) {
      if (i > 0) {
        *static_cast<char *>(stack.push(1)) = ',';
      }
      // 解析string
      stringify_string(
          stack, std::string_view{val->object[i].str, val->object[i].strLen});
      *static_cast<char *>(stack.push(1)) = ':';
      // 解析Node
      stringify_value(stack, val->object[i].v);
    }
    *static_cast<char *>(stack.push(1)) = '}';
    break;
  default:
    break;
  }
}

void TinyJson::stringify_string(InternalStack &stack, std::string_view sv) {
  size_t len = sv.size(), size;
  char *head = nullptr, *p = nullptr;
  p = head = static_cast<char *>(stack.push(size = len * 6 + 2));
  *p++ = '\"'; // 先压入"
  for (size_t i = 0; i < len; ++i) {
    auto ch = static_cast<unsigned char>(sv[i]);
    switch (ch) {
    case '\"':
      *p++ = '\\';
      *p++ = '\"';
      break;
    case '\\':
      *p++ = '\\';
      *p++ = '\\';
      break;
    case '\b':
      *p++ = '\\';
      *p++ = 'b';
      break;
    case '\f':
      *p++ = '\\';
      *p++ = 'f';
      break;
    case '\n':
      *p++ = '\\';
      *p++ = 'n';
      break;
    case '\r':
      *p++ = '\\';
      *p++ = 'r';
      break;
    case '\t':
      *p++ = '\\';
      *p++ = 't';
      break;
    default:
      *p++ = ch;
      break;
    }
  }

  *p++ = '\"';
  stack.pop(size - (p - head));
}

std::span<char> TinyJson::stringify(JsonValue *val) {
  InternalStack stack{};
  stringify_value(stack, val->root);
  std::span<char> s{stack.ptr, stack.top};
  stack.ptr = nullptr;
  return s;
}

void TinyJson::stringify_value(std::string &str, Node *val) {
  size_t i = 0, strLen;
  char *start;              // for stringify number
  std::to_chars_result ret; // for stringify number
  switch (val->type) {
  case JsonType::JSON_NULL:
    str += "null";
    break;
  case JsonType::JSON_TRUE:
    str += "true";
    break;
  case JsonType::JSON_FALSE:
    str += "false";
    break;
  case JsonType::JSON_NUMBER:
    strLen = str.size();
    str.append(32, 0); // number最长32字符
    start = str.data() + strLen;
    ret = std::to_chars(start, start + 32, val->number);
    str.resize(ret.ptr - str.data());
    break;
  case JsonType::JSON_STRING:
    stringify_string(str, std::string_view{val->str, val->strLen});
    break;
  case JsonType::JSON_ARRAY:
    str += '[';
    for (i = 0; i < val->arrLen; ++i) {
      if (i > 0) {
        str += ',';
      }
      stringify_value(str, val->arr[i]);
    }
    str += ']';
    break;
  case JsonType::JSON_OBJECT:
    str += '{';
    for (i = 0; i < val->objSize; ++i) {
      if (i > 0) {
        str += ',';
      }
      // 解析string
      stringify_string(
          str, std::string_view{val->object[i].str, val->object[i].strLen});
      str += ':';
      // 解析Node
      stringify_value(str, val->object[i].v);
    }
    str += '}';
    break;
  default:
    break;
  }
}

void TinyJson::stringify_string(std::string &str, std::string_view sv) {
  str += '\"';
  size_t len = sv.size();
  for (size_t i = 0; i < len; ++i) {
    auto ch = static_cast<unsigned char>(sv[i]);
    switch (ch) {
    case '\"':
      str += "\\\"";
      break;
    case '\\':
      str += "\\\\";
      break;
    case '\b':
      str += "\\\b";
      break;
    case '\f':
      str += "\\\f";
      break;
    case '\n':
      str += "\\\n";
      break;
    case '\r':
      str += "\\\r";
      break;
    case '\t':
      str += "\\\t";
      break;
    default:
      str += sv[i];
      break;
    }
  }

  str += '\"';
}

std::string TinyJson::stringify2(JsonValue *val) {
  std::string ret;
  stringify_value(ret, val->root);
  return ret;
}