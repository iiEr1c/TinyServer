set(HEADER_LIST "${TinyJson_SOURCE_DIR}/include/TinyJson.hpp")

add_library(TinyJson TinyJson.cpp ${HEADER_LIST})

# We need this directory, and users of our library will need it too
target_include_directories(TinyJson PUBLIC ../include)

# All users of this library will need at least C++11
target_compile_features(TinyJson PUBLIC cxx_std_20)

target_compile_options(TinyJson PRIVATE
            ${CXX_FLAGS}
            "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
            "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>")
#target_compile_options(TinyNet PRIVATE "-fsanitize=address" "-fsanitize=undefined")
target_link_options(TinyJson PRIVATE ${SANITIZE_FLAG})

target_link_libraries(TinyJson PRIVATE)