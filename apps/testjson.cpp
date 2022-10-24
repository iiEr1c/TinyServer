
#include "HttpServer/TinyJson/include/TinyJson.hpp"

int main() {
  TinyJson::JsonValue val;
  val.root->type = TinyJson::JsonType::JSON_OBJECT;
  val.root->objSize = 2;
  val.root->object = (TinyJson::KVnode *)::malloc(sizeof(TinyJson::KVnode) * 2);

  char *userId = (char *)::malloc(6);
  userId[0] = 'u';
  userId[1] = 's';
  userId[2] = 'e';
  userId[3] = 'r';
  userId[4] = 'I';
  userId[5] = 'd';

  std::string param("abcdefghi");
  char *userIdVal = (char *)::malloc(param.size());
  size_t len = 0;
  if (param[0] != '%') {
    for (size_t i = 0; i < param.size(); ++i) {
      userIdVal[i] = param[i];
    }
  }

  TinyJson::Node *v = (TinyJson::Node *)::malloc(sizeof(TinyJson::Node));
  v->type = TinyJson::JsonType::JSON_STRING;
  v->str = userIdVal;
  v->strLen = len;

  val.root->object[0].str = userId;
  val.root->object[0].strLen = 6;
  val.root->object[0].v = v;

  char *ip = (char *)malloc(8);
  ip[0] = 'i';
  ip[1] = 'p';
  std::string ipAddr = "127.0.0.1";
  char *ipaddr = (char *)malloc(ipAddr.size());
  for (size_t i = 0; i < ipAddr.size(); ++i) {
    ipaddr[i] = ipAddr[i];
  }

  TinyJson::Node *v2 = (TinyJson::Node *)::malloc(sizeof(TinyJson::Node));
  v2->type = TinyJson::JsonType::JSON_STRING;
  v2->str = ipaddr;
  v2->strLen = ipAddr.size();
  val.root->object[1].str = ip;
  val.root->object[1].strLen = 2;
  val.root->object[1].v = v2;

  TinyJson json;
  std::string body = json.stringify2(&val);
}