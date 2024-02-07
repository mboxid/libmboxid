cmake_minimum_required(VERSION 3.25)

project(libmboxid VERSION 0.0.1
    DESCRIPTION "A Modbus TCP C++ library for unixoid systems"
    LANGUAGES CXX
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(common)
add_library(mboxid $<TARGET_OBJECTS:common>)

add_subdirectory(tests)

