#include "platform.h"
#include "http.h"
#include "file.h"
#include "log.h"
#include "platform_signal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef _WIN32
    #include <sys/time.h>
#endif

#define MAX_HEADER_SIZE 8192
#define IDLE_TIMEOUT_SECONDS 10

// ---------------------------------------------------------------------------
// Small string helpers
// ---------------------------------------------------------------------------

static int strncmp_ci(const char* a, const char* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        int ca = tolower((unsigned char)a[i]);
        int cb = tolower((unsigned char)b[i]);
        if (ca != cb) return ca - cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

static int contains_ci(const char* haystack, const char* needle)
{
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++)
    {
        if (strncmp_ci(haystack, needle, nlen) == 0)
        {
            return 1;
        }
    }
    return 0;
}

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Find a header value by (case-insensitive) name. Returns 1 and copies the
// trimmed value into out if present, else 0.
static int get_header(const char* buffer, const char* name, char* out, size_t out_size)
{
    size_t namelen = strlen(name);
    const char* line = buffer;

    // Skip the request line; headers begin after the first CRLF.
    const char* first_crlf = strstr(buffer, "\r\n");
    if (!first_crlf) return 0;
    line = first_crlf + 2;

    while (line && *line && !(line[0] == '\r' && line[1] == '\n'))
    {
        if (strncmp_ci(line, name, namelen) == 0 && line[namelen] == ':')
        {
            const char* v = line + namelen + 1;
            while (*v == ' ' || *v == '\t') v++;
            size_t i = 0;
            while (v[i] && v[i] != '\r' && v[i] != '\n' && i + 1 < out_size)
            {
                out[i] = v[i];
                i++;
            }
            out[i] = '\0';
            return 1;
        }
        const char* nl = strstr(line, "\r\n");
        if (!nl) break;
        line = nl + 2;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Public parsing helpers (also unit-tested)
// ---------------------------------------------------------------------------

void parse_request_line(const char* request, char* method, int method_size, char* target, int target_size)
{
    method[0] = '\0';
    target[0] = '\0';

    // Find the end of the first line (e.g., "GET /index.html HTTP/1.1")
    const char* line_end = strstr(request, "\r\n");
    if (!line_end)
    {
        return;
    }

    int first_line_length = line_end - request;
    char temp[1024];
    if (first_line_length > (int)sizeof(temp) - 1)
    {
        first_line_length = sizeof(temp) - 1;
    }
    memcpy(temp, request, first_line_length);
    temp[first_line_length] = '\0';

    // Use width specifiers to prevent buffer overflow.
    char format[32];
    snprintf(format, sizeof(format), "%%%ds %%%ds", method_size - 1, target_size - 1);
    sscanf(temp, format, method, target);
}

int url_decode(const char* in, char* out, size_t out_size)
{
    size_t oi = 0;
    for (size_t i = 0; in[i] != '\0'; i++)
    {
        if (oi + 1 >= out_size)
        {
            return -1;  // not enough room (leave space for NUL)
        }

        char c = in[i];
        if (c == '%')
        {
            int hi = hex_value(in[i + 1]);
            int lo = (hi >= 0) ? hex_value(in[i + 2]) : -1;
            if (hi < 0 || lo < 0)
            {
                return -1;  // malformed escape
            }
            out[oi++] = (char)((hi << 4) | lo);
            i += 2;
        }
        else
        {
            out[oi++] = c;
        }
    }
    out[oi] = '\0';
    return 0;
}

int path_is_safe(const char* path)
{
    const char* p = path;
    while (*p)
    {
        // At the start of each path segment, reject "." == ".." (a parent ref).
        if (p[0] == '.' && p[1] == '.' && (p[2] == '\0' || p[2] == '/'))
        {
            return 0;
        }
        const char* slash = strchr(p, '/');
        if (!slash) break;
        p = slash + 1;
    }
    return 1;
}

int parse_range(const char* range_value, long* start, long* end)
{
    const char* p = range_value;
    while (*p == ' ' || *p == '\t') p++;

    if (strncmp(p, "bytes=", 6) != 0)
    {
        return 0;
    }
    p += 6;

    // Suffix form: "bytes=-N" (last N bytes).
    if (*p == '-')
    {
        p++;
        char* endp;
        long n = strtol(p, &endp, 10);
        if (endp == p || n <= 0) return 0;
        *start = -1;
        *end = n;
        return 1;
    }

    char* endp;
    long s = strtol(p, &endp, 10);
    if (endp == p || s < 0 || *endp != '-') return 0;
    p = endp + 1;

    // Open-ended form: "bytes=S-".
    if (*p == '\0' || *p == '\r' || *p == '\n' || *p == ',')
    {
        *start = s;
        *end = -1;
        return 1;
    }

    long e = strtol(p, &endp, 10);
    if (endp == p || e < s) return 0;
    *start = s;
    *end = e;
    return 1;
}

// ---------------------------------------------------------------------------
// Connection handling
// ---------------------------------------------------------------------------

static void set_recv_timeout(int client_fd, int seconds)
{
#ifdef _WIN32
    DWORD timeout = (DWORD)seconds * 1000;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
#else
    struct timeval tv;
    tv.tv_sec = seconds;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
}

// Read until the end-of-headers marker. Returns bytes read (>0), 0 on a clean
// close/timeout/partial request, or -2 if the header block exceeds the buffer.
static int read_headers(int client_fd, char* buffer, size_t buffer_size)
{
    size_t total = 0;
    while (total < buffer_size - 1)
    {
        int n = recv(client_fd, buffer + total, (int)(buffer_size - 1 - total), 0);
        if (n <= 0)
        {
            return 0;
        }
        total += (size_t)n;
        buffer[total] = '\0';
        if (strstr(buffer, "\r\n\r\n"))
        {
            return (int)total;
        }
    }
    return -2;  // headers too large
}

static int connection_keep_alive(const char* buffer)
{
    int http11 = (strstr(buffer, "HTTP/1.1") != NULL);
    char conn[64];
    if (get_header(buffer, "Connection", conn, sizeof(conn)))
    {
        if (contains_ci(conn, "close")) return 0;
        if (contains_ci(conn, "keep-alive")) return 1;
    }
    return http11;  // HTTP/1.1 defaults to keep-alive, HTTP/1.0 to close
}

static void send_redirect(int client_fd, const char* path, int keep_alive)
{
    char location[1100];
    snprintf(location, sizeof(location), "%s/", path);

    char body[256];
    int body_len = snprintf(body, sizeof(body),
        "<html><head><title>301 Moved Permanently</title></head>"
        "<body><h1>301 Moved Permanently</h1>"
        "<p><a href=\"%s\">%s</a></p></body></html>\n",
        location, location);

    char header[1500];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 301 Moved Permanently\r\n"
        "Location: %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: %s\r\n"
        "\r\n",
        location, body_len, keep_alive ? "keep-alive" : "close");

    send_all(client_fd, header, (size_t)header_len);
    send_all(client_fd, body, (size_t)body_len);
}

// Resolve the request to a filesystem path and serve it.
static void serve_request(int client_fd, const char* doc_root, const http_request_t* req)
{
    char file_path[1024];
    if (strcmp(req->path, "/") == 0)
    {
        snprintf(file_path, sizeof(file_path), "%s", doc_root);
    }
    else
    {
        snprintf(file_path, sizeof(file_path), "%s%s", doc_root, req->path);
    }

    if (is_directory(file_path))
    {
        // Redirect "/dir" -> "/dir/" so relative links inside resolve correctly.
        size_t plen = strlen(req->path);
        if (plen == 0 || req->path[plen - 1] != '/')
        {
            send_redirect(client_fd, req->path, req->keep_alive);
            return;
        }

        char index_path[1024];
        snprintf(index_path, sizeof(index_path), "%s/index.html", file_path);
        if (is_regular_file(index_path))
        {
            serve_file(client_fd, index_path, req);
        }
        else
        {
            serve_directory_listing(client_fd, file_path, req->path, req->keep_alive);
        }
    }
    else
    {
        serve_file(client_fd, file_path, req);
    }
}

// Process one request from an already-read header block. Returns 1 if the
// connection may be reused (keep-alive), 0 if it should be closed.
static int process_request(int client_fd, const char* doc_root, const char* buffer)
{
    http_request_t req;
    memset(&req, 0, sizeof(req));

    char target[1024];
    parse_request_line(buffer, req.method, sizeof(req.method), target, sizeof(target));

    if (req.method[0] == '\0' || target[0] == '\0')
    {
        send_error_response(client_fd, 400, "Bad Request", 0, NULL);
        return 0;
    }

    req.keep_alive = connection_keep_alive(buffer) && !g_shutdown_requested;

    // Range header (optional).
    char range_value[256];
    if (get_header(buffer, "Range", range_value, sizeof(range_value)))
    {
        if (parse_range(range_value, &req.range_start, &req.range_end))
        {
            req.has_range = 1;
        }
    }

    // Split off the query string, then percent-decode the path.
    char* qmark = strchr(target, '?');
    if (qmark)
    {
        strncpy(req.query, qmark + 1, sizeof(req.query) - 1);
        *qmark = '\0';
    }
    if (url_decode(target, req.path, sizeof(req.path)) != 0)
    {
        send_error_response(client_fd, 400, "Bad Request", 0, NULL);
        return 0;
    }

    LOG_DEBUG("Method: %s, Path: %s", req.method, req.path);

    // Only GET and HEAD are supported; close on others to avoid body desync.
    if (strcmp(req.method, "GET") != 0 && strcmp(req.method, "HEAD") != 0)
    {
        send_error_response(client_fd, 405, "Method Not Allowed", 0, "Allow: GET, HEAD\r\n");
        return 0;
    }

    if (!path_is_safe(req.path))
    {
        send_error_response(client_fd, 403, "Forbidden", req.keep_alive, NULL);
        return req.keep_alive;
    }

    serve_request(client_fd, doc_root, &req);
    return req.keep_alive;
}

void* handle_client_thread(void* context_ptr)
{
    client_context_t* context = (client_context_t*)context_ptr;
    int client_fd = context->client_fd;
    char doc_root[512];
    strncpy(doc_root, context->doc_root, sizeof(doc_root) - 1);
    doc_root[sizeof(doc_root) - 1] = '\0';
    free(context_ptr);

    handle_client(client_fd, doc_root);
    return NULL;
}

void handle_client(int client_fd, const char* doc_root)
{
    // Idle timeout so a kept-alive but silent connection releases its worker.
    set_recv_timeout(client_fd, IDLE_TIMEOUT_SECONDS);

    int keep_alive = 1;
    while (keep_alive && !g_shutdown_requested)
    {
        char buffer[MAX_HEADER_SIZE + 1];
        int header_len = read_headers(client_fd, buffer, sizeof(buffer));
        if (header_len == -2)
        {
            send_error_response(client_fd, 431, "Request Header Fields Too Large", 0, NULL);
            break;
        }
        if (header_len <= 0)
        {
            break;  // clean close, timeout, or partial request
        }

        LOG_DEBUG("Request received:\n%s", buffer);
        keep_alive = process_request(client_fd, doc_root, buffer);
    }

    closesocket(client_fd);
}
