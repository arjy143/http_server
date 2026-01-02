#include "platform.h"
#include "file.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* open_file(const char* file_path, int* out_size)
{
    FILE* file = fopen(file_path, "rb");
    if (!file)
    {
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (size < 0)
    {
        fclose(file);
        return NULL;
    }

    char* buffer = (char*)malloc(size);
    if (!buffer)
    {
        LOG_ERROR("Failed to allocate memory for file: %s", file_path);
        fclose(file);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, size, file);
    fclose(file);

    if ((long)read_bytes != size)
    {
        LOG_ERROR("Failed to read complete file: %s", file_path);
        free(buffer);
        return NULL;
    }

    *out_size = (int)size;
    return buffer;
}

void serve_file(int client_fd, const char* file_path)
{
    int file_size;
    char* file_data = open_file(file_path, &file_size);

    if (!file_data)
    {
        LOG_DEBUG("File not found: %s", file_path);
        char response[512];
        const char* not_found_body = "404 Not Found\n";
        snprintf(response, sizeof(response),
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %zu\r\n"
            "\r\n"
            "%s",
            strlen(not_found_body), not_found_body);

        send(client_fd, response, (int)strlen(response), 0);
        return;
    }

    const char* mime_type = get_mime_type(file_path);

    char header[512];
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        mime_type,
        file_size);

    send(client_fd, header, (int)strlen(header), 0);
    send(client_fd, file_data, file_size, 0);
    free(file_data);

    LOG_DEBUG("Served: %s (%d bytes, %s)", file_path, file_size, mime_type);
}

const char* get_mime_type(const char* file_path)
{
    const char* ext = strrchr(file_path, '.');
    if (!ext)
    {
        return "application/octet-stream";
    }

    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0)
    {
        return "text/html";
    }
    else if (strcmp(ext, ".css") == 0)
    {
        return "text/css";
    }
    else if (strcmp(ext, ".js") == 0)
    {
        return "application/javascript";
    }
    else if (strcmp(ext, ".png") == 0)
    {
        return "image/png";
    }
    else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0)
    {
        return "image/jpeg";
    }
    else if (strcmp(ext, ".gif") == 0)
    {
        return "image/gif";
    }
    else if (strcmp(ext, ".svg") == 0)
    {
        return "image/svg+xml";
    }
    else if (strcmp(ext, ".ico") == 0)
    {
        return "image/x-icon";
    }
    else if (strcmp(ext, ".txt") == 0)
    {
        return "text/plain";
    }
    else if (strcmp(ext, ".json") == 0)
    {
        return "application/json";
    }
    else if (strcmp(ext, ".pdf") == 0)
    {
        return "application/pdf";
    }
    else if (strcmp(ext, ".xml") == 0)
    {
        return "application/xml";
    }
    else if (strcmp(ext, ".woff") == 0)
    {
        return "font/woff";
    }
    else if (strcmp(ext, ".woff2") == 0)
    {
        return "font/woff2";
    }
    else
    {
        return "application/octet-stream";
    }
}
