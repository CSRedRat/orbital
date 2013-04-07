cmake_minimum_required(VERSION 2.8)
project(orbital)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(Wayland)

include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}
                    ${CMAKE_CURRENT_SOURCE_DIR}/src/effects
                    /usr/include/pixman-1)

add_subdirectory(src)
add_subdirectory(client)


