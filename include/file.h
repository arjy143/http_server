#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include "http.h"

// Loop send() until all bytes are written (handles partial writes).
// Returns total bytes sent, or -1 on error.
int send_all(int client_fd, const char* buf, size_t len);

// Send a minimal plain-text status response, e.g. "404 Not Found".
// extra_headers, if non-NULL, is inserted verbatim and must include its own
// trailing CRLF (e.g. "Allow: GET, HEAD\r\n").
void send_error_response(int client_fd, int code, const char* status_text, int keep_alive, const char* extra_headers);

// Stream a regular file to the client. Honours HEAD (headers only) and the
// Range fields carried on req (206 / 416). Sends 404 if not a regular file.
void serve_file(int client_fd, const char* file_path, const http_request_t* req);

// MIME type from file extension. Text types include "; charset=utf-8".
const char* get_mime_type(const char* file_path);

// Filesystem checks.
int is_directory(const char* path);
int is_regular_file(const char* path);

// Auto-generated HTML listing for a directory that has no index.html.
void serve_directory_listing(int client_fd, const char* dir_path, const char* url_path, int keep_alive);

#endif
