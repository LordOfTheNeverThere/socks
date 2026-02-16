cmake_minimum_required(VERSION 4.1)
project(socks VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(socks INTERFACE)
target_include_directories(socks INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(socks INTERFACE cxx_std_17)

add_executable(socks_dev
        src/main.cpp
        include/socks/AddressInfo.h
        include/socks/types.h
        include/socks/NetworkListener.h
)

target_link_libraries(socks_dev PRIVATE socks)


# GTEST

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(tests)