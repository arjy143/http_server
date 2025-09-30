#ifndef FILE_H
#define FILE_H

char* open_file(const char* file_path, int* out_size);
void serve_file(int client_fd, const char* file_path);
const char* get_mime_type(const char* file_path);

#endif