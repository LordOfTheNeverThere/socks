add_executable(socksTests NetworkListener.cpp
        RawSocket.cpp
        Tools.cpp)


target_link_libraries(socksTests
        PRIVATE
        socks
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(socksTests)
