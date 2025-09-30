#ifndef PLATFORM_H
#define PLATFORM_H

//block of platform specific includes to save space
#ifdef _WIN32
    #define _WIN32_WINNT 0x0601 
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <windows.h>

	//windows threading
	typedef HANDLE thread_t;
	#define THREAD_CREATE(thr, func, arg) do { \
		*(thr) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL); \
		if (*(thr) == NULL) { \
			perror("CreateThread failed"); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)
	#pragma comment(lib, "ws2_32.lib") //not necessary anymore
#else
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <netdb.h>
	//making linux use the same function name as windows
	#define closesocket close

	//posix threading
	#include <pthread.h>
	typedef pthread_t thread_t;
	#define THREAD_CREATE(thr, func, arg) do { \
		if (pthread_create((thr), NULL, (func), (arg)) != 0) { \
			perror("pthread_create failed"); \
			exit(EXIT_FAILURE); \
		} \
	} while(0)
#endif

#endif