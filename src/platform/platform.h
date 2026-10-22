// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


// Logger
void platform_consoleWrite(const char *message, LogLevel colour);
void platform_consoleWriteError(const char *message, LogLevel colour);

// Memory
bool readFromMemory(void *handle, uintptr_t address, size_t length, uint8_t *bytes);
uint8_t readByte(void *handle, uintptr_t address);
void writeToMemory(void *handle, uintptr_t address, size_t length, const uint8_t *bytes);

// Process
void platform_openProcess(ProcessContext *context);
void platform_closeProcess(ProcessContext *context);

// Paths
void platform_getExecutableDirectory(char *buffer, size_t size);
bool platform_getUserDirectory(char *buffer, size_t size, bool state);
FILE *platform_fopen(const char *path, const char *mode);
bool platform_removeFile(const char *path);
bool platform_replaceFile(const char *source, const char *destination);

// Time
int64_t platform_getMicroseconds(void);
