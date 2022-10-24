add_subdirectory(HttpServer)

#add_executable(testJson testjson.cpp)
#target_compile_features(testJson PRIVATE cxx_std_20)

# target_link_libraries(app PRIVATE TinyJson)
#target_link_libraries(testJson PRIVATE TinyNet TinyJson spdlog::spdlog)

#target_compile_options(testJson PRIVATE
#            ${CXX_FLAGS}
#            "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
#            "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>")

#target_compile_options(app PRIVATE "-fsanitize=address" "-fsanitize=undefined")
#target_link_options(testJson PRIVATE ${SANITIZE_FLAG})




#add_executable(test_server test_socket.cpp)
#target_compile_features(test_server PRIVATE cxx_std_20)

# target_link_libraries(app PRIVATE TinyJson)
#target_link_libraries(test_server PRIVATE TinyNet spdlog::spdlog)

#target_compile_options(test_server PRIVATE
#            ${CXX_FLAGS}
#            "$<$<CONFIG:Debug>:${CXX_FLAGS_DEBUG}>"
#            "$<$<CONFIG:Release>:${CXX_FLAGS_RELEASE}>")

#target_compile_options(app PRIVATE "-fsanitize=address" "-fsanitize=undefined")
#target_link_options(test_server PRIVATE ${SANITIZE_FLAG})