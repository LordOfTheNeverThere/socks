add_executable(socksTests NetworkListener.cpp
        Tools.cpp)


add_executable(sudoSocksTests
        RawSocket.cpp)


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


# Post-build command to grant raw socket capabilities
add_custom_command(TARGET sudoSocksTests POST_BUILD
        COMMAND sudo setcap cap_net_raw+ep $<TARGET_FILE:sudoSocksTests>
        COMMENT "Granting CAP_NET_RAW to sudoSocksTests binary"
        VERBATIM
)

add_test(NAME sudoSocksTests COMMAND sudoSocksTests)