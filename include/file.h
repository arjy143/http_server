#ifndef FILE_H
#define FILE_H

char* open_file(const char* file_path, int* out_size);
void serve_file(int client_fd, const char* file_path);
const char* get_mime_type(const char* file_path);

//function to check if path is a directory
int is_directory(const char* path);
void serve_directory_listing(int client_fd, const char* dir_path, const char* url_path);

#endif
