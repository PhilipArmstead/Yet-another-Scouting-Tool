// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform.h"


// Logging
#include <stdio.h>


// Fatal, error, warn, info, debug
static const char *colourStrings[5] = {"30;41", "1;31", "1;33", "1;32", "1;30"};

void platform_consoleWrite(const char *message, const LogLevel colour) {
	printf("\033[%sm%s\033[0m", colourStrings[colour], message);
}

void platform_consoleWriteError(const char *message, const LogLevel colour) {
	fprintf(stderr, "\033[%sm%s\033[0m", colourStrings[colour], message);
}

// Memory
#include <string.h>
#include <unistd.h>


bool readFromMemory(void *handle, const uintptr_t address, const size_t length, uint8_t *bytes) {
	if (pread((int)(intptr_t)handle, bytes, length, (off_t)address) != (ssize_t)length) {
		memset(bytes, 0, length);
		platform_consoleWriteError("Failed to read memory\n", LogLevelError);
		return false;
	}

	return true;
}

uint8_t readByte(void *handle, const uintptr_t address) {
	uint8_t byte = 0;
	if (pread((int)(intptr_t)handle, &byte, 1, (off_t)address) != 1) {
		platform_consoleWriteError("Failed to read byte\n", LogLevelError);
	}
	return byte;
}

void writeToMemory(void *handle, const uintptr_t address, const size_t length, const uint8_t *bytes) {
	if (pwrite((int)(intptr_t)handle, bytes, length, (off_t)address) != (ssize_t)length) {
		platform_consoleWriteError("Failed to write to memory\n", LogLevelError);
	}
}


// Process
#include <fcntl.h>
#include <stdlib.h>

#include "core/logger.h"


// Resolves the load base of fm.exe by locating the mapping that covers the given file offset.
// Returns 0 when no such mapping exists.
static uintptr_t findMemoryMap(const uint32_t pid, const uint64_t offset) {
	char mapsPath[64];
	snprintf(mapsPath, sizeof(mapsPath), "/proc/%u/maps", pid);

	FILE *f = fopen(mapsPath, "r");
	if (!f) {
		LOG_ERROR("Could not read memory map for PID %u", pid);
		return 0;
	}

	uintptr_t base = 0;
	char line[8192];
	while (fgets(line, sizeof(line), f)) {
		unsigned long start, end, fileOff;
		char path[4096];
		path[0] = '\0';

		// maps line format:
		// start-end perms offset dev inode pathname...
		// pathname can contain spaces, so we take the whole tail.
		if (sscanf(line, "%lx-%lx %*s %lx %*s %*s %4095[^\n]", &start, &end, &fileOff, path) < 4) {
			continue;
		}

		// Does the requested file offset fall in this mapping?
		if (offset < (uintptr_t)fileOff) {
			continue;
		}

		if (offset >= (uintptr_t)fileOff + (uintptr_t)(end - start)) {
			continue;
		}

		base = (uintptr_t)start - (uintptr_t)fileOff;
		break;
	}


	fclose(f);
	return base;
}

void platform_openProcess(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;
	context->moduleBaseAddress = 0;

	FILE *fp = popen("pidof -s 'Main Thread' 2>/dev/null", "r");
	if (!fp) {
		LOG_ERROR("Failed to enumerate processes");
		return;
	}

	char output[64];
	const bool read = fgets(output, sizeof(output), fp) != NULL;
	pclose(fp);

	const uint32_t pid = read ? (uint32_t)strtoul(output, NULL, 10) : 0;
	if (pid == 0) {
		LOG_WARN("Process '%s' not found", "fm.exe");
		return;
	}

	char memoryPath[32];
	snprintf(memoryPath, sizeof(memoryPath), "/proc/%u/mem", pid);
	const int handle = open(memoryPath, O_RDWR);
	if (handle == -1) {
		LOG_ERROR("Failed to open process with PID %u: access denied or process not found", pid);
		return;
	}

	const uintptr_t baseAddress = findMemoryMap(pid, CURRENT_DATETIME_PTR_BASE);
	if (baseAddress == 0) {
		LOG_ERROR("Module '%s' not found in process with PID %u", "fm.exe", pid);
		close(handle);
		return;
	}

	context->handle = (void*)(intptr_t)handle;
	context->pid = pid;
	context->moduleBaseAddress = baseAddress;
}

// Time
#include <time.h>


int64_t platform_getMicroseconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Paths
void platform_getExecutableDirectory(char *buffer, const size_t size) {
	if (size == 0) {
		return;
	}

	const ssize_t length = readlink("/proc/self/exe", buffer, size - 1);
	if (length <= 0) {
		platform_consoleWriteError("Could not resolve executable path\n", LogLevelError);
		buffer[0] = '\0';
		return;
	}

	buffer[length] = '\0';

	// Strip the executable filename, leaving the directory.
	char *lastSlash = strrchr(buffer, '/');
	if (lastSlash) {
		lastSlash[1] = '\0';
	}
}
