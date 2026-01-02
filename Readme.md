# HTTP Server

This project is a simple HTTP server built using C and CMake as the build system. Compatible for both Windows and Linux.

## Features

DONE:
- basic parsing of http headers
- serves a variety of files
- gets mime type of files and adjusts accordingly
- supports windows and linux
- does some sanitisation of url
- uses a generic thread pool to distribute connection handling

TO DO:
- parse http headers properly
- persistent connections
- extend the thread pool - currently uses basic locks, could change to non blocking concurrency? epoll?
- https?
- websocket?

## Build Instructions

1. Clone the repository:
    ```bash
    git clone https://github.com/your-username/http_server.git
    cd http_server
    ```

2. Create a build directory and configure the project:
    ```bash
    mkdir build
    cd build
    cmake ..
    ```

3. Build the project:
    ```bash
    cmake --build .
    ```

## Usage

After building, you can run the server:
```bash
./http_server
```
 