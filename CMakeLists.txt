cmake_minimum_required(VERSION 3.25)

project(libmboxid VERSION 0.0.1
    DESCRIPTION "A Modbus TCP C++ library for unixoid systems."
    LANGUAGES C CXX
    )

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(examples)

include(CMakePackageConfigHelpers)
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/mboxidConfig.cmake
    INSTALL_DESTINATION lib/cmake/mboxid
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/mboxidConfigVersion.cmake
    VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    COMPATIBILITY SameMajorVersion
    )

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mboxidConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/mboxidConfigVersion.cmake
    DESTINATION lib/cmake/mboxid
    )
