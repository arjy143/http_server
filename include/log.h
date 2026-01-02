#ifndef LOG_H
#define LOG_H

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

// Initialize logging system
void log_init(int verbose);

// Log a message with the given level
void log_msg(log_level_t level, const char* format, ...);

// Convenience macros
#define LOG_DEBUG(...) log_msg(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...)  log_msg(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...)  log_msg(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_msg(LOG_LEVEL_ERROR, __VA_ARGS__)

#endif
