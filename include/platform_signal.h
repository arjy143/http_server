#ifndef PLATFORM_SIGNAL_H
#define PLATFORM_SIGNAL_H

#ifdef _WIN32
    #include <windows.h>
#else
    #include <signal.h>
#endif

// Global flag for shutdown - set by signal handler
extern volatile int g_shutdown_requested;

// Initialize signal handlers (cross-platform)
void signal_init(void);

#endif
