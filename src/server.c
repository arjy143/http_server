#include "platform.h"
#include "platform_threading.h"
#include "server.h"
#include "http.h"
#include "config.h"
#include "log.h"
#include "platform_signal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread_pool.h"

void run_server(const server_config_t* config)
{
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0)
        {
            LOG_ERROR("Failed to initialize WinSock");
            return;
        }
    #endif

    log_init(config->verbose);
    signal_init();

    struct sockaddr_in address;
    int addr_len = sizeof(address);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd <= 0)
    {
        LOG_ERROR("Socket creation failed");
        return;
    }

    // Allow socket reuse to avoid "Address already in use" errors
    int opt = 1;
    #ifdef _WIN32
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    #else
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(config->port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        LOG_ERROR("Bind failed on port %d", config->port);
        closesocket(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0)
    {
        LOG_ERROR("Listen failed");
        closesocket(server_fd);
        return;
    }

    LOG_INFO("Server listening on port %d", config->port);
    LOG_INFO("Document root: %s", config->doc_root);
    LOG_INFO("Worker threads: %d", config->num_threads);

    thread_pool_t* thread_pool = thread_pool_create(config->num_threads, config->queue_size);
    if (!thread_pool)
    {
        LOG_ERROR("Failed to create thread pool");
        closesocket(server_fd);
        return;
    }

    while (!g_shutdown_requested)
    {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addr_len);
        if (client_fd < 0)
        {
            if (g_shutdown_requested)
            {
                break;
            }
            LOG_WARN("Accept failed");
            continue;
        }

        // Get client IP for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, sizeof(client_ip));
        LOG_DEBUG("Connection from %s", client_ip);

        // Create context for the worker thread
        client_context_t* context = malloc(sizeof(client_context_t));
        if (!context)
        {
            LOG_ERROR("Failed to allocate client context");
            closesocket(client_fd);
            continue;
        }
        context->client_fd = client_fd;
        strncpy(context->doc_root, config->doc_root, sizeof(context->doc_root) - 1);
        context->doc_root[sizeof(context->doc_root) - 1] = '\0';

        if (thread_pool_add_task(thread_pool, handle_client_thread, (void*)context) != 0)
        {
            LOG_WARN("Thread pool queue full, dropping connection");
            closesocket(client_fd);
            free(context);
        }
        // Note: Socket is NOT closed here - worker thread will close it after handling
    }

    LOG_INFO("Shutting down server...");
    thread_pool_destroy(thread_pool);
    closesocket(server_fd);

    #ifdef _WIN32
        WSACleanup();
    #endif

    LOG_INFO("Server stopped");
}
