#pragma once
#include <string_view>
#include <unordered_map>

using namespace std::string_view_literals;

namespace TinyNet {

enum HttpMethod { INVALID, GET, PUT, POST, DELETE };

static inline std::unordered_map<HttpMethod, std::string_view> HttpMethodMsg = {
    {HttpMethod::GET, "GET"sv},
    {HttpMethod::PUT, "PUT"},
    {HttpMethod::POST, "POST"},
    {HttpMethod::DELETE, "DELETE"}};

enum HttpVersion { Unknown, Http11 };

enum HttpStatus {
  STATUS_CODE_INIT = 0,
  STATUS_CODE_200 = 200,
  STATUS_CODE_301 = 301,
  STATUS_CODE_400 = 400,
  STATUS_CODE_404 = 404,
  STATUS_CODE_500 = 500
};

static inline std::unordered_map<HttpStatus, std::string_view> StatusCodeMsg = {
    {STATUS_CODE_200, "OK"sv},
    {STATUS_CODE_301, "Moved Permanently"sv},
    {STATUS_CODE_400, "Bad Request"sv},
    {STATUS_CODE_404, "Not Found"sv},
    {STATUS_CODE_500, "Internal Server Error"sv}};

}; // namespace TinyNet