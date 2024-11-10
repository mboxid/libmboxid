include(FetchContent)

FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG e69e5f9 # v10.2.1
    )
FetchContent_MakeAvailable(fmt)

configure_file(version.cpp.in version.cpp)

add_library(mboxid
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
    error.cpp
    logger.cpp
    network.cpp
    modbus_tcp_server.cpp
    modbus_tcp_server_impl.cpp
    modbus_tcp_client.cpp
    modbus_protocol_common.cpp
    modbus_protocol_server.cpp
    modbus_protocol_client.cpp
    crc32.c
    )
target_include_directories(mboxid PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    )
target_link_libraries(mboxid PRIVATE fmt)

file(GLOB PUBLIC_HEADERS LIST_DIRECTORIES false CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/include/mboxid/*.hpp)
    
target_sources(mboxid PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES ${PUBLIC_HEADERS}
    )

install(TARGETS mboxid EXPORT mboxidTargets FILE_SET public_headers)
install(EXPORT mboxidTargets DESTINATION lib/cmake/mboxid)
