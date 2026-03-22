add_executable(socksTests NetworkListener.cpp
        Tools.cpp
)


add_executable(sudoSocksTests
        RawSocket.cpp
        LocalHost.cpp
)

target_link_libraries(socksTests
        PRIVATE
        socks
        GTest::gtest_main
)

target_link_libraries(sudoSocksTests
        PRIVATE
        socks
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(socksTests)
gtest_discover_tests(sudoSocksTests)
