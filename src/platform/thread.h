#pragma once
#include <stdbool.h>

typedef struct {
	void *native;
} Thread;
typedef struct {
	void *native;
} Mutex;

bool thread_start(Thread *thread, void (*function)(void *), void *argument);
void thread_join(Thread *thread);
bool mutex_init(Mutex *mutex);
void mutex_lock(Mutex *mutex);
void mutex_unlock(Mutex *mutex);
void mutex_destroy(Mutex *mutex);
void thread_sleep(unsigned milliseconds);
