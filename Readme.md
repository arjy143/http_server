# http_server

This project is a simple HTTP server built using C and CMake as the build system. Compatible for both Windows and Linux.It's designed to be simple to use with 0 configuration required.

## Features

- **Cross-platform** - Works on Windows and Linux
- **Zero dependencies** - Single executable, no runtime dependencies
- **Multi-threaded** - Thread pool handles concurrent connections efficiently
- **Static file serving** - Serves HTML, CSS, JS, images, and more
- **Directory listing** - Auto-generates file listing when no index.html exists
- **Configurable** - CLI options for port, directory, threads, and verbosity
- **Graceful shutdown** - Clean exit on Ctrl+C
- **Security** - Path sanitization prevents directory traversal attacks

## Quick Start

```bash
# Serve current directory on port 8080
./http_server

# Serve a specific directory
./http_server -d /var/www/html

# Use a different port
./http_server -p 3000

# Enable verbose logging
./http_server -v
```

Then open http://localhost:8080 in your browser.

## Download

Check releases page.

## Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `-p, --port PORT` | Port to listen on | 8080 |
| `-d, --directory DIR` | Document root directory | Current directory |
| `-t, --threads NUM` | Number of worker threads | 4 |
| `-v, --verbose` | Enable verbose logging | Off |
| `-h, --help` | Show help message | |
| `--version` | Show version | |

## Examples

### Serve a static website
```bash
./http_server -d ./my-website -p 80
```

### Development server with logging
```bash
./http_server -d ./src -p 3000 -v
```

### High-traffic configuration
```bash
./http_server -t 16 -p 8080 -d /var/www
```

## Building from Source

### Requirements
- CMake 3.10+
- C11 compatible compiler (GCC, Clang, MSVC)

### Build Steps

```bash
git clone https://github.com/your-username/http_server.git
cd http_server
mkdir build && cd build
cmake ..
cmake --build .
```

The executable will be at `build/http_server` (or `build/Release/http_server.exe` on Windows).

### Platform Abstraction

Cross-platform compatibility is achieved through header-only abstractions:

- `platform.h` - Socket APIs (WinSock2 / POSIX)
- `platform_threading.h` - Threading primitives (Win32 / pthreads)
- `platform_signal.h` - Signal handling (SetConsoleCtrlHandler / sigaction)

## Supported MIME Types

| Extension | MIME Type |
|-----------|-----------|
| `.html`, `.htm` | text/html |
| `.css` | text/css |
| `.js` | application/javascript |
| `.json` | application/json |
| `.png` | image/png |
| `.jpg`, `.jpeg` | image/jpeg |
| `.gif` | image/gif |
| `.svg` | image/svg+xml |
| `.ico` | image/x-icon |
| `.pdf` | application/pdf |
| `.txt` | text/plain |
| `.xml` | application/xml |
| `.woff`, `.woff2` | font/woff, font/woff2 |
