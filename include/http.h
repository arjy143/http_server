#ifndef HTTP_H
#define HTTP_H

void* handle_client(void* client_fd_ptr)
void parse_request_line(const char* request, char* method, int method_size, char* path, int path_size);
int sanitise_path(const char* path);

#endif