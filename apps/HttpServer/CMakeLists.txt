set(HTTP_HEADER_LIST
    "HttpCommon.hpp"
    "HttpServer.hpp"
    "HttpContext.hpp"
    "HttpRequest.hpp"
    "HttpResponse.hpp"
    "HttpRouter.hpp")

set(HTTP_SRC_LIST
    "HttpServer.cpp"
    "HttpContext.cpp"
    "HttpRequest.cpp"
    "HttpResponse.cpp")

add_subdirectory(TinyJson)

add_executable(httpServerApp httpServerApp.cpp ${HTTP_SRC_LIST} ${HTTP_HEADER_LIST})

target_compile_features(httpServerApp PRIVATE cxx_std_20)


target_compile_options(httpServerApp PRIVATE
            ${CXX_FLAGS}
            "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
            "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>")

#target_compile_options(app PRIVATE "-fsanitize=address" "-fsanitize=undefined")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_options(httpServerApp PRIVATE ${SANITIZE_FLAG})
endif()

# target_link_libraries(app PRIVATE TinyJson)
target_link_libraries(httpServerApp PRIVATE TinyNet TinyJson spdlog::spdlog)