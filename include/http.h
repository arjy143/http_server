#ifndef HTTP_H
#define HTTP_H

// Client context passed to worker threads
typedef struct {
    int client_fd;
    char doc_root[512];
} client_context_t;

void* handle_client_thread(void* context_ptr);
void handle_client(int client_fd, const char* doc_root);
void parse_request_line(const char* request, char* method, int method_size, char* path, int path_size);
int sanitise_path(const char* path);

#endif
