cmake_minimum_required(VERSION 3.15)

project(TinyJson VERSION 0.1
                 DESCRIPTION "TinyJson"
                 LANGUAGES CXX)

include(FetchContent)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()


list(APPEND CXX_FLAGS_DEBUG
            "-O0"
            "-DDBUG")
            #"-Wsuggest-final-types"
            #"-Wsuggest-final-methods"
            #"-Wsuggest-override")

list(APPEND CXX_FLAGS_RELEASE
            "-O3"
            "-Wall"
            "-Wextra"
            "-Werror"
            "-Wconversion"
            "-Wno-unused-parameter"
            "-Wno-unused")

set(CMAKE_VERBOSE_MAKEFILE ON)

#enable_testing()

# The compiled library code is here
add_subdirectory(src)

# The executable code is here
add_subdirectory(apps)

enable_testing()
#if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    add_subdirectory(tests)
endif()
