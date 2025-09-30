#include "platform.h"
#include "file.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* open_file(const char* file_path, int* out_size)
{
	//read binary
	FILE* file = fopen(file_path, "rb");
	if (!file)
	{
		return NULL;
	}

	//get file size
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	//back to start
	fseek(file, 0, SEEK_SET);

	if (size < 0)
	{
		return NULL;
	}

	char* buffer = (char*)malloc(size);
	if (!buffer)
	{
		fclose(file);
		return NULL;
	}

	int read_bytes = fread(buffer, 1, size, file);
	fclose(file);

	if (read_bytes != size)
	{
		free(buffer);
		return NULL;
	}

	*out_size = size;
	return buffer;

}


void serve_file(int client_fd, const char* file_path)
{
	int file_size;
	char* file_data = open_file(file_path, &file_size);
	

	if (!file_data)
	{
		char response[512];
		const char* not_found_body = "404 not found\n";
		snprintf(response, sizeof(response),
			"HTTP/1.1 404 Not Found\r\n"
			"Content-Type: text/plain\r\n"
			"Content-Length: %zu\r\n"
			"\r\n"
			"%s",
			strlen(not_found_body), not_found_body);

		send(client_fd, response, strlen(response), 0);
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
	
	send(client_fd, header, strlen(header), 0);
	send(client_fd, file_data, file_size, 0);
	free(file_data);
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
	else
	{
		return "application/octet-stream";
	}
}