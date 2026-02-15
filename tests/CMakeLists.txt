add_executable(unit_tests SocketInterfaces.cpp)


target_link_libraries(unit_tests
        PRIVATE
        socks
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
