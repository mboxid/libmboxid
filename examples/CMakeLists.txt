
add_executable(server server.cpp)
target_link_libraries(server mboxid)

add_executable(client client.cpp)
target_link_libraries(client mboxid)
