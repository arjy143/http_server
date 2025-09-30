#include "platform.h"
#include "http.h"
#include "file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BUFFER_SIZE 1024

void* handle_client_thread(void* client_fd_ptr)
{
   //ugly but had to do this to make threading work properly
    int client_fd = *(int*)client_fd_ptr;
    free(client_fd_ptr);
    handle_client(client_fd);
    return NULL;
}


void handle_client(int client_fd)
{
	char buffer[BUFFER_SIZE] = {0};
	int read_val = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);
	if (read_val < 0)
	{
		perror("read");
		return;
	}

	buffer[read_val] = '\0';

	printf("Request recieved:\n%s\n", buffer);
	
	char method[16];
	char path[256];
	parse_request_line(buffer, method, sizeof(method), path, sizeof(path));

	printf("Method: %s, Path: %s\n", method, path);

	//deciding what to serve
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

		send(client_fd, response, strlen(response), 0);
		return;
	}

	if ((strcmp(path, "/") == 0))
	{
		serve_file(client_fd, "www/index.html");
	}
	else
	{
		char file_path[512];
    	snprintf(file_path, sizeof(file_path), "www%s", path);
    	serve_file(client_fd, file_path);

	}
	return;
}

//need to extend this to parse the header fully
void parse_request_line(const char* request, char* method, int method_size, char* path, int path_size)
{
	//e.g. GET/index.html would be the first line
	const char* line_end = strstr(request, "\r\n");
	if (!line_end)
	{
		method[0] = '\0';
		path[0] = '\0';
		return;
	}

	int first_line_length = line_end - request;
	char temp[256];
	if (first_line_length > sizeof(temp) -1)
	{
		first_line_length = sizeof(temp) -1;
	}
	memcpy(temp, request, first_line_length);
	temp[first_line_length] = '\0';

	//split the line into words
	sscanf(temp, "%s %s", method, path);
}



int sanitise_path(const char* path)
{
	if (strstr(path, "..") != NULL)
	{
		return 0;
	}
	return 1;
}