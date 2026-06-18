#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

// Client context passed to worker threads
typedef struct {
    int client_fd;
    char doc_root[512];
} client_context_t;

// Parsed view of an incoming request (only the parts we act on).
typedef struct http_request {
    char method[16];
    char path[1024];   // URL-decoded, query string stripped
    char query[1024];  // raw query (without '?'), may be empty
    int keep_alive;    // 1 if the connection should be reused after this response
    int has_range;     // 1 if a usable Range header was parsed
    // Range convention (resolved against file size by serve_file):
    //   range_start >= 0, range_end >= 0  -> bytes=start-end
    //   range_start >= 0, range_end <  0  -> bytes=start-     (start to EOF)
    //   range_start <  0, range_end >= 0  -> bytes=-N         (last N bytes)
    long range_start;
    long range_end;
} http_request_t;

void* handle_client_thread(void* context_ptr);
void handle_client(int client_fd, const char* doc_root);

// Parse "GET /path?q=1 HTTP/1.1" -> method + raw target (path with query intact).
void parse_request_line(const char* request, char* method, int method_size, char* target, int target_size);

// Decode %XX escapes in a URL path. '+' is left literal (it is a valid path
// character; only query strings treat it as space). Returns 0 on success and a
// NUL-terminated result in out; -1 on malformed input or insufficient space.
int url_decode(const char* in, char* out, size_t out_size);

// 1 if the path contains no ".." segment (safe), 0 otherwise. Call after
// url_decode so percent-encoded traversal (e.g. %2e%2e) is also caught.
int path_is_safe(const char* path);

// Parse a "Range:" header value like "bytes=0-99". Returns 1 and fills
// start/end (see convention above) for a single byte range, else 0.
int parse_range(const char* range_value, long* start, long* end);

#endif
