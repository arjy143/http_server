#include "platform.h"
#include "file.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
#endif

// MSVC's <sys/stat.h> doesn't define these POSIX helpers.
#ifndef S_ISDIR
    #define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
    #define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif

int send_all(int client_fd, const char* buf, size_t len)
{
    size_t total = 0;
    while (total < len)
    {
        int n = send(client_fd, buf + total, (int)(len - total), SEND_FLAGS);
        if (n <= 0)
        {
            return -1;
        }
        total += (size_t)n;
    }
    return (int)total;
}

void send_error_response(int client_fd, int code, const char* status_text, int keep_alive, const char* extra_headers)
{
    char body[128];
    int body_len = snprintf(body, sizeof(body), "%d %s\n", code, status_text);

    char header[512];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "%s"
        "Connection: %s\r\n"
        "\r\n",
        code, status_text, body_len,
        extra_headers ? extra_headers : "",
        keep_alive ? "keep-alive" : "close");

    send_all(client_fd, header, (size_t)header_len);
    send_all(client_fd, body, (size_t)body_len);
}

static void send_range_not_satisfiable(int client_fd, long long file_size, int keep_alive)
{
    char extra[64];
    snprintf(extra, sizeof(extra), "Content-Range: bytes */%lld\r\n", file_size);
    send_error_response(client_fd, 416, "Range Not Satisfiable", keep_alive, extra);
}

int is_directory(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

int is_regular_file(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return 0;
    }
    return S_ISREG(st.st_mode);
}

void serve_directory_listing(int client_fd, const char* dir_path, const char* url_path, int keep_alive)
{
    // Start building HTML response
    char* html = malloc(65536);  // 64KB buffer for directory listing
    if (!html)
    {
        LOG_ERROR("Failed to allocate memory for directory listing");
        send_error_response(client_fd, 500, "Internal Server Error", 0, NULL);
        return;
    }

    int offset = 0;
    offset += snprintf(html + offset, 65536 - offset,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <title>Index of %s</title>\n"
        "  <style>\n"
        "    body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; margin: 40px; }\n"
        "    h1 { color: #333; border-bottom: 1px solid #ddd; padding-bottom: 10px; }\n"
        "    table { border-collapse: collapse; width: 100%%; max-width: 800px; }\n"
        "    th, td { text-align: left; padding: 8px 12px; }\n"
        "    th { background: #f5f5f5; }\n"
        "    tr:hover { background: #f9f9f9; }\n"
        "    a { color: #0066cc; text-decoration: none; }\n"
        "    a:hover { text-decoration: underline; }\n"
        "    .dir { font-weight: bold; }\n"
        "    .dir::before { content: '\\1F4C1 '; }\n"
        "    .file::before { content: '\\1F4C4 '; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <h1>Index of %s</h1>\n"
        "  <table>\n"
        "    <tr><th>Name</th><th>Type</th></tr>\n",
        url_path, url_path);

    // Add parent directory link if not at root
    if (strcmp(url_path, "/") != 0)
    {
        offset += snprintf(html + offset, 65536 - offset,
            "    <tr><td><a href=\"..\">..</a></td><td>Parent Directory</td></tr>\n");
    }

#ifdef _WIN32
    // Windows directory listing
    WIN32_FIND_DATAA find_data;
    char search_path[1024];
    snprintf(search_path, sizeof(search_path), "%s\\*", dir_path);

    HANDLE hFind = FindFirstFileA(search_path, &find_data);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // Skip . and ..
            if (strcmp(find_data.cFileName, ".") == 0 || strcmp(find_data.cFileName, "..") == 0)
            {
                continue;
            }

            int is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            const char* type_class = is_dir ? "dir" : "file";
            const char* type_name = is_dir ? "Directory" : "File";
            const char* slash = is_dir ? "/" : "";

            offset += snprintf(html + offset, 65536 - offset,
                "    <tr><td class=\"%s\"><a href=\"%s%s\">%s%s</a></td><td>%s</td></tr>\n",
                type_class, find_data.cFileName, slash, find_data.cFileName, slash, type_name);

        } while (FindNextFileA(hFind, &find_data) != 0 && offset < 60000);

        FindClose(hFind);
    }
#else
    // POSIX directory listing
    DIR* dir = opendir(dir_path);
    if (dir)
    {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL && offset < 60000)
        {
            // Skip . and ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            {
                continue;
            }

            // Check if it's a directory
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            int is_dir = is_directory(full_path);

            const char* type_class = is_dir ? "dir" : "file";
            const char* type_name = is_dir ? "Directory" : "File";
            const char* slash = is_dir ? "/" : "";

            offset += snprintf(html + offset, 65536 - offset,
                "    <tr><td class=\"%s\"><a href=\"%s%s\">%s%s</a></td><td>%s</td></tr>\n",
                type_class, entry->d_name, slash, entry->d_name, slash, type_name);
        }
        closedir(dir);
    }
#endif

    offset += snprintf(html + offset, 65536 - offset,
        "  </table>\n"
        "  <hr>\n"
        "  <p><small>http_server</small></p>\n"
        "</body>\n"
        "</html>\n");

    // Send HTTP response
    char header[256];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: %s\r\n"
        "\r\n",
        offset, keep_alive ? "keep-alive" : "close");

    send_all(client_fd, header, (size_t)header_len);
    send_all(client_fd, html, (size_t)offset);
    free(html);

    LOG_DEBUG("Served directory listing: %s", dir_path);
}

void serve_file(int client_fd, const char* file_path, const http_request_t* req)
{
    struct stat st;
    if (stat(file_path, &st) != 0 || !S_ISREG(st.st_mode))
    {
        LOG_DEBUG("File not found: %s", file_path);
        send_error_response(client_fd, 404, "Not Found", req->keep_alive, NULL);
        return;
    }

    long long file_size = (long long)st.st_size;
    const char* mime_type = get_mime_type(file_path);
    int is_head = (strcmp(req->method, "HEAD") == 0);

    long long start = 0;
    long long end = file_size - 1;  // inclusive
    int partial = 0;

    if (req->has_range)
    {
        if (req->range_start < 0)
        {
            // Suffix range: last range_end bytes.
            long long suffix = (long long)req->range_end;
            if (suffix <= 0)
            {
                send_range_not_satisfiable(client_fd, file_size, req->keep_alive);
                return;
            }
            if (suffix > file_size) suffix = file_size;
            start = file_size - suffix;
            end = file_size - 1;
        }
        else
        {
            start = (long long)req->range_start;
            end = (req->range_end < 0) ? file_size - 1 : (long long)req->range_end;
            if (end >= file_size) end = file_size - 1;
            if (file_size == 0 || start >= file_size || start > end)
            {
                send_range_not_satisfiable(client_fd, file_size, req->keep_alive);
                return;
            }
        }
        partial = 1;
    }

    long long content_length = partial ? (end - start + 1) : file_size;

    char header[512];
    int header_len;
    if (partial)
    {
        header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 206 Partial Content\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lld\r\n"
            "Content-Range: bytes %lld-%lld/%lld\r\n"
            "Accept-Ranges: bytes\r\n"
            "Connection: %s\r\n"
            "\r\n",
            mime_type, content_length, start, end, file_size,
            req->keep_alive ? "keep-alive" : "close");
    }
    else
    {
        header_len = snprintf(header, sizeof(header),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: %s\r\n"
            "Content-Length: %lld\r\n"
            "Accept-Ranges: bytes\r\n"
            "Connection: %s\r\n"
            "\r\n",
            mime_type, content_length,
            req->keep_alive ? "keep-alive" : "close");
    }

    if (send_all(client_fd, header, (size_t)header_len) < 0)
    {
        return;
    }

    // HEAD requests and empty bodies carry no payload.
    if (is_head || content_length == 0)
    {
        LOG_DEBUG("Served headers: %s (%lld bytes, %s)", file_path, content_length, mime_type);
        return;
    }

    FILE* file = fopen(file_path, "rb");
    if (!file)
    {
        // Headers already sent; can't recover gracefully, just stop writing.
        LOG_ERROR("Failed to open file after stat: %s", file_path);
        return;
    }

    if (start > 0)
    {
        fseek(file, (long)start, SEEK_SET);
    }

    char buf[65536];
    long long remaining = content_length;
    while (remaining > 0)
    {
        size_t want = remaining < (long long)sizeof(buf) ? (size_t)remaining : sizeof(buf);
        size_t got = fread(buf, 1, want, file);
        if (got == 0)
        {
            break;
        }
        if (send_all(client_fd, buf, got) < 0)
        {
            break;
        }
        remaining -= (long long)got;
    }
    fclose(file);

    LOG_DEBUG("Served: %s (%lld bytes, %s)", file_path, content_length, mime_type);
}

const char* get_mime_type(const char* file_path)
{
    const char* ext = strrchr(file_path, '.');
    if (!ext)
    {
        return "application/octet-stream";
    }

    // Text formats carry an explicit charset so browsers render them correctly.
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css") == 0)                              return "text/css; charset=utf-8";
    if (strcmp(ext, ".js") == 0 || strcmp(ext, ".mjs") == 0)   return "application/javascript; charset=utf-8";
    if (strcmp(ext, ".json") == 0 || strcmp(ext, ".map") == 0) return "application/json; charset=utf-8";
    if (strcmp(ext, ".txt") == 0)                              return "text/plain; charset=utf-8";
    if (strcmp(ext, ".xml") == 0)                              return "application/xml; charset=utf-8";

    if (strcmp(ext, ".svg") == 0)                              return "image/svg+xml";
    if (strcmp(ext, ".png") == 0)                              return "image/png";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".gif") == 0)                              return "image/gif";
    if (strcmp(ext, ".webp") == 0)                             return "image/webp";
    if (strcmp(ext, ".ico") == 0)                              return "image/x-icon";

    if (strcmp(ext, ".pdf") == 0)                              return "application/pdf";
    if (strcmp(ext, ".wasm") == 0)                             return "application/wasm";

    if (strcmp(ext, ".mp4") == 0)                              return "video/mp4";
    if (strcmp(ext, ".webm") == 0)                             return "video/webm";
    if (strcmp(ext, ".ogg") == 0)                              return "audio/ogg";
    if (strcmp(ext, ".mp3") == 0)                              return "audio/mpeg";
    if (strcmp(ext, ".wav") == 0)                              return "audio/wav";

    if (strcmp(ext, ".woff") == 0)                             return "font/woff";
    if (strcmp(ext, ".woff2") == 0)                            return "font/woff2";
    if (strcmp(ext, ".ttf") == 0)                              return "font/ttf";
    if (strcmp(ext, ".otf") == 0)                              return "font/otf";

    return "application/octet-stream";
}
