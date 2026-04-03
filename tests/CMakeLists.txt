add_executable(socksTests
        NetworkListener.cpp
        Tools.cpp
        RawSocket.cpp
        LocalHost.cpp
)


target_link_libraries(socksTests
        PRIVATE
        socks
        GTest::gtest_main
)

target_compile_definitions(socksTests PRIVATE
        SOCKS_RESOURCES="${CMAKE_BINARY_DIR}/resources/"
)

include(GoogleTest)

gtest_discover_tests(socksTests)
