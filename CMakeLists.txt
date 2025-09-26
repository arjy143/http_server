cmake_minimum_required(VERSION 3.10)

project(http_server C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_executable(http_server src/http_server.c) 
if (WIN32)
	target_link_libraries(http_server PRIVATE ws2_32)
endif()