cmake_minimum_required(VERSION 3.16)
project(con_simulator C)

set(CMAKE_C_STANDARD 99)

add_executable(client_simulator
        src/client_simulator.c
        src/con_common.c
        src/con_common.h
        src/client_simulator.h)

add_executable(server_simulator
        src/server_simulator.c
        src/con_common.c
        src/con_common.h
        src/server_simulator.h)

target_link_libraries(client_simulator pthread)
target_link_libraries(server_simulator pthread)
