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


# Post-build command to grant raw socket capabilities
add_custom_command(TARGET socksTests POST_BUILD
        COMMAND sudo setcap cap_net_raw+ep $<TARGET_FILE:socksTests>
        COMMENT "Granting CAP_NET_RAW to socksTests binary"
        VERBATIM
)