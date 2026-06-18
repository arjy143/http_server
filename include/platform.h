#ifndef PLATFORM_H
#define PLATFORM_H

//block of platform specific includes to save space
//specifically for networking
#ifdef _WIN32
    #define _WIN32_WINNT 0x0601
	#include <winsock2.h>
	#include <ws2tcpip.h>
	// Windows send() never raises SIGPIPE, so no extra flags are needed.
	#define SEND_FLAGS 0
#else
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <netdb.h>
	//making linux use the same function name as windows
	#define closesocket close
	// Avoid SIGPIPE when writing to a peer that has hung up (belt-and-suspenders
	// alongside ignoring SIGPIPE in signal_init).
	#ifdef MSG_NOSIGNAL
		#define SEND_FLAGS MSG_NOSIGNAL
	#else
		#define SEND_FLAGS 0
	#endif
#endif

#endif