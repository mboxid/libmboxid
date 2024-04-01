include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY git@github.com:google/googletest.git
    GIT_TAG        f8d7d77 # v1.14
    )

option(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(TESTS test_unique_fd test_byteorder test_error test_version test_logger
    test_modbus_tcp_server test_network test_modbus_protocol_common
    test_modbus_protocol_server
    )

include(GoogleTest)

foreach(TEST ${TESTS})
    add_executable(${TEST} ${TEST}.cpp)
    target_link_libraries(${TEST} mboxid GTest::gtest_main GTest::gmock_main
        fmt)
    target_include_directories(${TEST} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    gtest_discover_tests(${TEST})
endforeach()