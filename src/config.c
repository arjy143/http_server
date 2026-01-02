#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void config_init(server_config_t* config)
{
    config->port = DEFAULT_PORT;
    config->num_threads = DEFAULT_THREADS;
    config->queue_size = DEFAULT_QUEUE_SIZE;
    strncpy(config->doc_root, DEFAULT_DOC_ROOT, sizeof(config->doc_root) - 1);
    config->doc_root[sizeof(config->doc_root) - 1] = '\0';
    config->verbose = 0;
    config->show_help = 0;
    config->show_version = 0;
}

int config_parse_args(server_config_t* config, int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0)
        {
            if (i + 1 < argc)
            {
                config->port = atoi(argv[++i]);
                if (config->port <= 0 || config->port > 65535)
                {
                    fprintf(stderr, "Error: Invalid port number (must be 1-65535)\n");
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "Error: --port requires a value\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--directory") == 0)
        {
            if (i + 1 < argc)
            {
                strncpy(config->doc_root, argv[++i], sizeof(config->doc_root) - 1);
                config->doc_root[sizeof(config->doc_root) - 1] = '\0';
            }
            else
            {
                fprintf(stderr, "Error: --directory requires a value\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0)
        {
            if (i + 1 < argc)
            {
                config->num_threads = atoi(argv[++i]);
                if (config->num_threads <= 0 || config->num_threads > 64)
                {
                    fprintf(stderr, "Error: Thread count must be 1-64\n");
                    return -1;
                }
            }
            else
            {
                fprintf(stderr, "Error: --threads requires a value\n");
                return -1;
            }
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
        {
            config->verbose = 1;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            config->show_help = 1;
        }
        else if (strcmp(argv[i], "--version") == 0)
        {
            config->show_version = 1;
        }
        else
        {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Use --help for usage information\n");
            return -1;
        }
    }
    return 0;
}

void config_print_help(const char* program_name)
{
    printf("Usage: %s [OPTIONS]\n\n", program_name);
    printf("A simple cross-platform HTTP server.\n\n");
    printf("Options:\n");
    printf("  -p, --port PORT       Port to listen on (default: %d)\n", DEFAULT_PORT);
    printf("  -d, --directory DIR   Document root directory (default: current directory)\n");
    printf("  -t, --threads NUM     Number of worker threads (default: %d)\n", DEFAULT_THREADS);
    printf("  -v, --verbose         Enable verbose logging\n");
    printf("  -h, --help            Show this help message\n");
    printf("      --version         Show version information\n");
}

void config_print_version(void)
{
    printf("http_server version %s\n", SERVER_VERSION);
}
