cmake_minimum_required(VERSION 3.10)

project(http_server C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(include)

add_executable(http_server
    src/main.c
    src/server.c
    src/http.c
    src/file.c
    src/thread_pool.c
    src/config.c
    src/log.c
    src/signal.c)

if (WIN32)
    target_link_libraries(http_server PRIVATE ws2_32)
endif()

# Test executable
add_executable(test_http_server
    tests/test_http_server.c
    src/http.c
    src/file.c
    src/config.c
    src/log.c)

target_include_directories(test_http_server PRIVATE tests)

if (WIN32)
    target_link_libraries(test_http_server PRIVATE ws2_32)
endif()

# Enable testing
enable_testing()
add_test(NAME http_server_tests COMMAND test_http_server)
