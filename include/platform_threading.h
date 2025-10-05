#ifndef PLATFORM_THREADING_H
#define PLATFORM_THREADING_H

//creating a platform independent threading interface
#ifdef _WIN32
	#include <windows.h>

	//windows threading
	typedef HANDLE thread_t;
    typedef CRITICAL_SECTION mutex_t;
    typedef CONDITION_VARIABLE cond_t;
    
	#define THREAD_CREATE(thr, func, arg) \
		*(thr) = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)(func), (arg), 0, NULL)
    #define THREAD_JOIN(thr) WaitForSingleObject((thr), INFINITE)
    #define THREAD_DETACH(thr) CloseHandle(thr)
    #define MUTEX_INIT(m) InitializeCriticalSection(m)
    #define MUTEX_LOCK(m) EnterCriticalSection(m)
    #define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
    #define MUTEX_DESTROY(m) DeleteCriticalSection(m)
    #define COND_INIT(c) InitializeConditionVariable(c)
    #define COND_WAIT(c, m) SleepConditionVariableCS(c, m, INFINITE)
    #define COND_SIGNAL(c) WakeConditionVariable(c)
    #define COND_BROADCAST(c) WakeAllConditionVariable(c)
    #define COND_DESTROY(c) //no neeed for this on windows
#else
	//posix threading
	#include <pthread.h>
	typedef pthread_t thread_t;
    typedef pthread_mutex_t mutex_t;
    typedef pthread_cond_t cond_t;
    
	#define THREAD_CREATE(thr, func, arg) pthread_create((thr), NULL, (func), (arg))
    #define THREAD_JOIN(thr) pthread_join((thr), NULL)
    #define THREAD_DETACH(thr) pthread_detach(thr)
    #define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
    #define MUTEX_LOCK(m) pthread_mutex_lock(m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
    #define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
    #define COND_INIT(c) pthread_cond_init(c, NULL)
    #define COND_WAIT(c, m) pthread_cond_wait(c, m)
    #define COND_SIGNAL(c) pthread_cond_signal(c)
    #define COND_BROADCAST(c) pthread_cond_broadcast(c)
    #define COND_DESTROY(c) pthread_cond_destroy(c)
#endif

#endif