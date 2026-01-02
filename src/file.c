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

int is_directory(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return 0;
    }
    return S_ISDIR(st.st_mode);
}

void serve_directory_listing(int client_fd, const char* dir_path, const char* url_path)
{
    // Start building HTML response
    char* html = malloc(65536);  // 64KB buffer for directory listing
    if (!html)
    {
        LOG_ERROR("Failed to allocate memory for directory listing");
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
    snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        offset);

    send(client_fd, header, (int)strlen(header), 0);
    send(client_fd, html, offset, 0);
    free(html);

    LOG_DEBUG("Served directory listing: %s", dir_path);
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
