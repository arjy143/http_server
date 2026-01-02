#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
#endif

static int g_verbose = 0;

void log_init(int verbose)
{
    g_verbose = verbose;
}

void log_msg(log_level_t level, const char* format, ...)
{
    // Skip debug messages unless verbose mode is enabled
    if (level == LOG_LEVEL_DEBUG && !g_verbose)
    {
        return;
    }

    const char* level_str;
    switch (level)
    {
        case LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;
        case LOG_LEVEL_INFO:  level_str = "INFO";  break;
        case LOG_LEVEL_WARN:  level_str = "WARN";  break;
        case LOG_LEVEL_ERROR: level_str = "ERROR"; break;
        default:              level_str = "?";     break;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    // Print timestamp and level
    fprintf(stderr, "[%s] [%-5s] ", time_buf, level_str);

    // Print the actual message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    fprintf(stderr, "\n");
    fflush(stderr);
}
