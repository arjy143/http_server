#include "platform.h"
#include "http.h"
#include "file.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

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
    char buffer[BUFFER_SIZE] = {0};
    int read_val = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
    if (read_val < 0)
    {
        LOG_ERROR("Failed to read from client socket");
        closesocket(client_fd);
        return;
    }

    buffer[read_val] = '\0';

    LOG_DEBUG("Request received:\n%s", buffer);

    char method[16];
    char path[256];
    parse_request_line(buffer, method, sizeof(method), path, sizeof(path));

    LOG_DEBUG("Method: %s, Path: %s", method, path);

    // Check for directory traversal attack
    if (sanitise_path(path) == 0)
    {
        char response[512];
        const char* forbidden_body = "403 Forbidden\n";
        snprintf(response, sizeof(response),
            "HTTP/1.1 403 Forbidden\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s",
            strlen(forbidden_body), forbidden_body);

        send(client_fd, response, (int)strlen(response), 0);
        closesocket(client_fd);
        return;
    }

    // Build file path from document root
    char file_path[1024];
    if (strcmp(path, "/") == 0)
    {
        snprintf(file_path, sizeof(file_path), "%s/index.html", doc_root);
    }
    else
    {
        snprintf(file_path, sizeof(file_path), "%s%s", doc_root, path);
    }

    serve_file(client_fd, file_path);

    // Close the socket after handling the request
    closesocket(client_fd);
}

void parse_request_line(const char* request, char* method, int method_size, char* path, int path_size)
{
    // Initialize outputs
    method[0] = '\0';
    path[0] = '\0';

    // Find the end of the first line (e.g., "GET /index.html HTTP/1.1")
    const char* line_end = strstr(request, "\r\n");
    if (!line_end)
    {
        return;
    }

    int first_line_length = line_end - request;
    char temp[512];
    if (first_line_length > (int)sizeof(temp) - 1)
    {
        first_line_length = sizeof(temp) - 1;
    }
    memcpy(temp, request, first_line_length);
    temp[first_line_length] = '\0';

    // Use width specifiers to prevent buffer overflow
    // method_size - 1 and path_size - 1 to leave room for null terminator
    char format[32];
    snprintf(format, sizeof(format), "%%%ds %%%ds", method_size - 1, path_size - 1);
    sscanf(temp, format, method, path);
}

int sanitise_path(const char* path)
{
    // Block directory traversal attempts
    if (strstr(path, "..") != NULL)
    {
        return 0;
    }
    return 1;
}
