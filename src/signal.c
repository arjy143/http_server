#include "platform_signal.h"
#include <stdio.h>

volatile int g_shutdown_requested = 0;

#ifdef _WIN32

BOOL WINAPI console_handler(DWORD signal)
{
    if (signal == CTRL_C_EVENT || signal == CTRL_BREAK_EVENT)
    {
        printf("\nShutdown requested (Ctrl+C)...\n");
        g_shutdown_requested = 1;
        return TRUE;
    }
    return FALSE;
}

void signal_init(void)
{
    SetConsoleCtrlHandler(console_handler, TRUE);
}

#else

#include <string.h>
#include <unistd.h>

static void signal_handler(int sig)
{
    (void)sig;
    const char msg[] = "\nShutdown requested (Ctrl+C)...\n";
    // Use write() in signal handler - it's async-signal-safe
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    g_shutdown_requested = 1;
}

void signal_init(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

#endif
