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

	char body[512];
	snprintf(body, sizeof(body), "You requested %s with method %s", path, method);

	char response[1024];
	snprintf(response, sizeof(response),
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/plain\r\n"
		"Content-Length: %zu\r\n"
		"\r\n"
		"%s",
		strlen(body), body);
	
	send(client_fd, response, strlen(response), 0);
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