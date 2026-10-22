#include "thread.h"
#include "core/logger.h"
#include <stdlib.h>
#ifdef ARCH_WIN
#include <process.h>
#include <windows.h>
#else
#include <errno.h>
#include <pthread.h>
#include <time.h>
#endif

typedef struct {
	void (*function)(void *);
	void *argument;
#ifdef ARCH_WIN
	HANDLE handle;
#else
	pthread_t handle;
#endif
} NativeThread;

#ifdef ARCH_WIN
static unsigned __stdcall execute(void *argument) {
#else
static void *execute(void *argument) {
#endif
	NativeThread *thread = argument;
	thread->function(thread->argument);
	return 0;
}

bool thread_start(Thread *thread, void (*function)(void *), void *argument) {
	NativeThread *native = malloc(sizeof(*native));
	if (!native) {
		LOG_ERROR("Could not allocate worker thread");
		return false;
	}
	*native = (NativeThread){.function = function, .argument = argument};
#ifdef ARCH_WIN
	native->handle = (HANDLE)_beginthreadex(NULL, 0, execute, native, 0, NULL);
	const bool failed = native->handle == NULL;
#else
	const bool failed = pthread_create(&native->handle, NULL, execute, native) != 0;
#endif
	if (failed) {
		LOG_ERROR("Could not start worker thread");
		free(native);
		return false;
	}
	thread->native = native;
	return true;
}

void thread_join(Thread *thread) {
	NativeThread *native = thread->native;
	if (!native)
		return;
#ifdef ARCH_WIN
	WaitForSingleObject(native->handle, INFINITE);
	CloseHandle(native->handle);
#else
	pthread_join(native->handle, NULL);
#endif
	free(native);
	thread->native = NULL;
}

bool mutex_init(Mutex *mutex) {
#ifdef ARCH_WIN
	SRWLOCK *lock = malloc(sizeof(*lock));
	if (lock)
		InitializeSRWLock(lock);
#else
	pthread_mutex_t *lock = malloc(sizeof(*lock));
	if (lock && pthread_mutex_init(lock, NULL) != 0) {
		free(lock);
		lock = NULL;
	}
#endif
	mutex->native = lock;
	if (!lock)
		LOG_ERROR("Could not initialize mutex");
	return lock != NULL;
}

void mutex_lock(Mutex *mutex) {
#ifdef ARCH_WIN
	AcquireSRWLockExclusive(mutex->native);
#else
	pthread_mutex_lock(mutex->native);
#endif
}

void mutex_unlock(Mutex *mutex) {
#ifdef ARCH_WIN
	ReleaseSRWLockExclusive(mutex->native);
#else
	pthread_mutex_unlock(mutex->native);
#endif
}

void mutex_destroy(Mutex *mutex) {
	if (!mutex->native)
		return;
#ifndef ARCH_WIN
	pthread_mutex_destroy(mutex->native);
#endif
	free(mutex->native);
	mutex->native = NULL;
}

void thread_sleep(unsigned milliseconds) {
#ifdef ARCH_WIN
	Sleep(milliseconds);
#else
	struct timespec delay = {.tv_sec = milliseconds / 1000, .tv_nsec = (long)(milliseconds % 1000) * 1000000L};
	while (nanosleep(&delay, &delay) != 0 && errno == EINTR) {
	}
#endif
}
