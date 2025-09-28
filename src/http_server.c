#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

void handle_client(int client_fd);
void parse_request_line(const char* request, char* method, int method_size, char* path, int path_size);
char* open_file(const char* file_path, int* out_size);
void serve_file(int client_fd, const char* file_path);
const char* get_mime_type(const char* file_path);
int sanitise_path(const char* path);

int main()
{
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
	{
		printf("failed to init winsock");
		return 1;
	}
	
	struct sockaddr_in address;
	int addr_len = sizeof(address);
	char buffer[BUFFER_SIZE] = {0};
	
	//IPv4, TCP
	int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd <= 0)
	{
		perror("Socket failed.");
		exit(EXIT_FAILURE);
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);
	
	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
	{
		perror("Bind failed.");
		exit(EXIT_FAILURE);
	}
	
	if (listen(server_fd, 3) > 0)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	
	printf("Server listening on port %d...\n", PORT);
	
	
	while (1)
	{
		int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
		if (client_fd < 0)
		{
			perror("accept");
			continue;
		}
		
		printf("client connected");
		
		handle_client(client_fd);
		closesocket(client_fd);
		printf("client disconnected");
	}
	
	closesocket(server_fd);
	WSACleanup();
	return 0;
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
	
}

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

int sanitise_path(const char* path)
{
	if (strstr(path, "..") != NULL)
	{
		return 0;
	}
	return 1;
}