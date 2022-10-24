#pragma once
#include "HttpCommon.hpp"
#include "logger.hpp"
#include <cassert>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include <vector>

namespace TinyNet {

struct m_string_view {
  const char *data;
  unsigned short length;
};

// std::ostream &operator<<(std::ostream &os, m_string_view &sv) {
//   os << std::string(sv.data, sv.length);
//   return os;
// }

template <class UserDataType> class HttpRouter {
private:
  struct Node {
    std::string name;
    std::map<std::string, Node *> childrens;
    short handler;
  };

  std::vector<std::function<void(UserDataType, std::vector<m_string_view> &)>>
      handlers;
  std::vector<m_string_view> params;

  Node *GETTree = new Node("GET", {}, -1);
  std::string get_serialize_tree;

  template <std::size_t httpMethod>
  void addToTrie(std::vector<std::string> route, short handler) {
    Node *parent = nullptr;
    if constexpr (httpMethod == HttpMethod::GET) {
      parent = GETTree;
    } else if constexpr (httpMethod == HttpMethod::POST) {
      // ...
    }

    assert(parent != nullptr);

    for (std::string &node : route) {
      if (parent->childrens.find(node) == parent->childrens.end()) {
        parent->childrens[node] = new Node(node, {}, handler);
      }
      parent = parent->childrens[node];
    }
  }

  /**
   * 在序列化Trie时, 内存布局如下
   * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   * +  cur Trie length and     + cur Trie.name +  function  + cur Trie.name +
   * + all children Trie length + length[strLen]+    index   +  real content +
   * +       2 bytes            +    2 bytes    +   2 bytes  + prev [strLen] +
   * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
   */
  template <size_t httpMethod> unsigned short serialize_tree(Node *n) {
    unsigned short nodeLength =
        6 + static_cast<unsigned short>(n->name.length());
    for (const auto &c : n->childrens) {
      nodeLength += serialize_tree<httpMethod>(c.second);
    }

    unsigned short nodeNameLength =
        static_cast<unsigned short>(n->name.length());
    std::string serializeStr;
    serializeStr.append((char *)&nodeLength, sizeof(nodeLength));
    serializeStr.append((char *)&nodeNameLength, sizeof(nodeNameLength));
    serializeStr.append((char *)&n->handler, sizeof(n->handler));
    serializeStr.append(n->name.data(), n->name.length());

    if constexpr (httpMethod == HttpMethod::GET) {
      get_serialize_tree = serializeStr + get_serialize_tree;
    } else if constexpr (httpMethod == HttpMethod::POST) {
      // todo
    }
    return nodeLength;
  }

  inline const char *find_node(const char *parent_node, const char *name,
                               unsigned short name_length) {
    unsigned short nodeLength, nodeNameLength;
    ::memcpy(&nodeLength, parent_node, 2);
    ::memcpy(&nodeNameLength, parent_node + 2, 2);
    const char *stop = parent_node + nodeLength;
    for (const char *candidate = parent_node + 6 + nodeNameLength;
         candidate < stop;) {
      // unsigned short curNodeLength = *(unsigned short *)&candidate[0];
      // unsigned short curNodeNameLength = *(unsigned short *)&candidate[2];
      unsigned short curNodeLength, curNodeNameLength;
      ::memcpy(&curNodeLength, candidate, 2);
      ::memcpy(&curNodeNameLength, candidate + 2, 2);

      if (candidate[6] == ':') {
        if (name_length == 0) [[unlikely]] {
          return nullptr;
        }
        params.push_back(m_string_view{.data = name, .length = name_length});
        return candidate;
      } else if (curNodeNameLength == name_length &&
                 !memcmp(candidate + 6, name, name_length)) {

        return candidate;
      }

      candidate += curNodeLength;
    }

    return nullptr;
  }

  inline const char *getNextSegment(const char *start, const char *end) {
    const char *stop =
        static_cast<const char *>(memchr(start, '/', end - start));
    return stop != nullptr ? stop : end;
  }

  template <std::size_t httpMethod>
  inline int lookup(const char *url, size_t length) {

    url++;
    length--;

    const char *treeStart;
    if constexpr (httpMethod == HttpMethod::GET) {
      treeStart = get_serialize_tree.data();
    } else if constexpr (httpMethod == HttpMethod::POST) {
      // ...
    }

    const char *stop, *start = url, *end_ptr = url + length;
    do {

      stop = getNextSegment(start, end_ptr);

      if (start == stop && stop != end_ptr) [[unlikely]] {
        return -1;
      }
      if (nullptr ==
          (treeStart = find_node(treeStart, start,
                                 static_cast<unsigned short>(stop - start)))) {
        return -1;
      }

      start = stop + 1;
    } while (start < end_ptr);

    short nodeLen, nodeStrLen, index;
    ::memcpy(&nodeLen, treeStart, 2);
    ::memcpy(&nodeStrLen, treeStart + 2, 2);
    ::memcpy(&index, treeStart + 4, 2);
    if (nodeLen == nodeStrLen + 6) [[likely]] {

      return index;
    } else {
      return -1;
    }
  }

public:
  HttpRouter() { params.reserve(128); }

  ~HttpRouter() {
    std::queue<Node *> que;

    if (GETTree != nullptr) {
      que.push(GETTree);
      while (!que.empty()) {
        Node *tmp = que.front();
        que.pop();
        for (const auto &c : tmp->childrens) {
          que.push(c.second);
        }
        delete tmp;
      }
    }

    // 类似地添加POST/DELETE等
  }

  template <std::size_t httpMethod>
  HttpRouter *
  add(const char *pattern,
      std::function<void(UserDataType, std::vector<m_string_view> &)> handler) {

    if (pattern[0] == '/') [[likely]] {
      pattern++;
    }

    std::vector<std::string> nodes;
    const char *stop, *start = pattern, *end_ptr = pattern + strlen(pattern);
    do {
      stop = getNextSegment(start, end_ptr);
      nodes.push_back(std::string(start, stop - start));
      start = stop + 1;
    } while (start < end_ptr);

    addToTrie<httpMethod>(std::move(nodes),
                          static_cast<short>(handlers.size()));
    handlers.emplace_back(std::move(handler));

    serialize<httpMethod>();
    return this;
  }

  template <std::size_t httpMethod> void serialize() {
    if constexpr (httpMethod == HttpMethod::GET) {
      get_serialize_tree.clear();
      serialize_tree<httpMethod>(GETTree);
    } else if constexpr (httpMethod == HttpMethod::POST) {
      // ...
    } else {
      // ...
    }
  }

  template <std::size_t httpMethod>
  void route(const std::string url, UserDataType userData) {

    int funcIdx =
        lookup<httpMethod>(url.data(), static_cast<int>(url.length()));
    LDebug("url= [{}], funcIdx = [{}]", url, funcIdx);

    if (funcIdx >= 0) [[likely]] {
      handlers[funcIdx](userData, params);
      params.clear();
    } else {
      // 404
      handlers[lookup<httpMethod>("/404.html", 9)](userData, params);
      params.clear();
    }
  }
};

}; // namespace TinyNet