#ifndef CONFIG_H
#define CONFIG_H

#define DEFAULT_PORT 8080
#define DEFAULT_THREADS 4
#define DEFAULT_QUEUE_SIZE 64
#define DEFAULT_DOC_ROOT "."
#define SERVER_VERSION "1.0.0"

typedef struct {
    int port;
    int num_threads;
    int queue_size;
    char doc_root[512];
    int verbose;
    int show_help;
    int show_version;
} server_config_t;

// Initialize config with defaults
void config_init(server_config_t* config);

// Parse command line arguments (cross-platform, no getopt)
int config_parse_args(server_config_t* config, int argc, char* argv[]);

// Print help message
void config_print_help(const char* program_name);

// Print version info
void config_print_version(void);

#endif
