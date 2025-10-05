
#include "platform.h"
#include "platform_threading.h"
#include "server.h"
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread_pool.h"



#define BUFFER_SIZE 1024


void run_server(int port)
{
	#ifdef _WIN32
		//windows specific init
		WSADATA wsa;
		if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
		{
			printf("failed to init winsock");
			return;
		}
	#endif
	
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
	address.sin_port = htons(port);
	
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
	
	printf("Server listening on port %d...\n", port);

	//thread pool that takes 4 threads and 10 tasks
	thread_pool_t thread_pool = thread_pool_create(4, 10);
	
	while (1)
	{
		int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
		if (client_fd < 0)
		{
			perror("accept");
			continue;
		}
		int* client_fd_ptr = malloc(sizeof(int));
		*client_fd_ptr = client_fd;

		printf("client connected");

		thread_pool_add_task(thread_pool, handle_client_thread, client_fd_ptr);

		#ifndef _WIN32
    		pthread_detach(thread);
		#endif
	
		closesocket(client_fd);
		printf("client disconnected");
	}
	
	thread_pool_destroy(&thread_pool);
	closesocket(server_fd);

	#ifdef _WIN32
		//windows specific cleanup
		WSACleanup();
	#endif
}

// void handle_client(int client_fd)
// {
// 	const char* msg = "hello";
//     send(client_fd, msg, (int)strlen(msg), 0);

// }