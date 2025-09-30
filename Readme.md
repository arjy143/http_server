# HTTP Server

This project is a simple HTTP server built using C and CMake as the build system. Compatible for both Windows and Linux.

## Features

- Handles basic HTTP requests and serves a variety of files
- TO DO: parse http headers properly, maybe do https using openssl?, websocket, in built cache, thread pool or a non blocking type of concurrency

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
