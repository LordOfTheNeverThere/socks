cmake_minimum_required(VERSION 4.1)
project(socks VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Construct Macro with Path to resources folder
set(SOCKS_RESOURCE_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/socks/resources")
configure_file(
        include/socks/cmakeVars.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/generated/socks/cmakeVars.h
)


# Define the Lib
add_library(socks INTERFACE)

target_include_directories(socks INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        # Also include the generated folder so code can find <socks/config.h>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(socks INTERFACE cxx_std_17)

add_executable(socks_dev
        src/main.cpp
        include/socks/AddressInfo.h
        include/socks/types.h
        include/socks/NetworkListener.h
        include/socks/Exceptions.h
        include/socks/L4Socket.h
        include/socks/RawSocket.h
        include/socks/Tools.h
        include/socks/Socket.h
        include/socks/IPv4Header.h
        include/socks/IPv4ToMACARPPacket.h
        include/socks/LocalHost.h
        include/socks/InternalInterface.h
        include/socks/ExternalInterface.h
        include/socks/cmakeVars.h.in
)
target_link_libraries(socks_dev PRIVATE socks)

# This makes sure the dev app and tests find the files in the build folder
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
# We copy resources into the tests folder so they can run locally
#file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/tests")
# GTEST

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()
add_subdirectory(tests)

# --- INSTALLATION LOGIC ---

# Install the actual resource files
install(DIRECTORY resources/ DESTINATION "${SOCKS_RESOURCE_INSTALL_DIR}")

# Install the headers so other apps can find them
install(DIRECTORY include/socks DESTINATION include)

# Allow this project to be "exported" for find_package()
install(TARGETS socks EXPORT socksTargets)
install(EXPORT socksTargets
        FILE socksTargets.cmake
        NAMESPACE socks::
        DESTINATION lib/cmake/socks
)