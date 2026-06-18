# http_server

This project is a simple HTTP server built using C and CMake as the build system. Compatible for both Windows and Linux. It's designed to be simple to use with 0 configuration required.

## Features

- **Cross-platform** - Works on Windows and Linux
- **Zero dependencies** - Single executable, no runtime dependencies
- **Multi-threaded** - A worker thread pool handles concurrent connections (the same
  model `cpp-httplib` uses - stays simple and fully cross-platform)
- **Streaming file serving** - Files are streamed in 64 KB chunks, so memory stays flat
  regardless of file size
- **HTTP keep-alive** - Reuses connections (HTTP/1.1), with an idle timeout so silent
  connections don't pin a worker
- **Range requests** - `206 Partial Content` support, so browsers can seek in
  `<video>`/`<audio>` and resume downloads
- **HEAD support** - Returns headers only; unsupported methods get `405 Method Not Allowed`
- **Static file serving** - Serves HTML, CSS, JS, images, fonts, media, and more, with
  correct MIME types and charset
- **Directory listing** - Auto-generates a file listing when no index.html exists, and
  redirects `/dir` to `/dir/` so relative links resolve
- **Configurable** - CLI options for port, directory, threads, and verbosity
- **Graceful shutdown** - Clean exit on Ctrl+C
- **Security** - Paths are URL-decoded before a `..`-segment check, blocking directory
  traversal (including percent-encoded `%2e%2e` attempts)

Also unit tested using my custom unit testing library, https://github.com/arjy143/Attest.

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

Text types are served with `; charset=utf-8`.

| Extension | MIME Type |
|-----------|-----------|
| `.html`, `.htm` | text/html |
| `.css` | text/css |
| `.js`, `.mjs` | application/javascript |
| `.json`, `.map` | application/json |
| `.xml` | application/xml |
| `.txt` | text/plain |
| `.png` | image/png |
| `.jpg`, `.jpeg` | image/jpeg |
| `.gif` | image/gif |
| `.webp` | image/webp |
| `.svg` | image/svg+xml |
| `.ico` | image/x-icon |
| `.pdf` | application/pdf |
| `.wasm` | application/wasm |
| `.mp4` | video/mp4 |
| `.webm` | video/webm |
| `.ogg` | audio/ogg |
| `.mp3` | audio/mpeg |
| `.wav` | audio/wav |
| `.woff`, `.woff2` | font/woff, font/woff2 |
| `.ttf`, `.otf` | font/ttf, font/otf |
