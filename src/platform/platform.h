// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"

#include <unistd.h>


#ifdef ARCH_WIN
#include <windows.h>

typedef HANDLE thread_t;
typedef HANDLE event_t;
#endif
#ifdef ARCH_LINUX
#include <pthread.h>
#include <semaphore.h>

typedef pthread_t thread_t;
typedef sem_t event_t;
#endif
#ifdef ARCH_MACOS
#include <pthread.h>
#include <semaphore.h>

typedef mach_port_t thread_t;
typedef sem_t event_t;
#endif


// Logger
void platform_consoleWrite(const char *message, LogLevel colour);
void platform_consoleWriteError(const char *message, LogLevel colour);

// Memory
bool readFromMemory(void *handle, uintptr_t address, size_t length, uint8_t *bytes);
uint8_t readByte(void *handle, uintptr_t address);
void writeToMemory(void *handle, uintptr_t address, size_t length, const uint8_t *bytes);

// Process
void platform_openProcess(ProcessContext *context);

// Paths
void platform_getExecutableDirectory(char *buffer, size_t size);

// Time
int64_t platform_getMicroseconds(void);
