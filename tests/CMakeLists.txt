add_executable(socksTests NetworkListener.cpp)


target_link_libraries(socksTests
        PRIVATE
        socks
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(socksTests)
