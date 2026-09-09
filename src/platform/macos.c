// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform.h"


#if defined(ARCH_MACOS)
// Logging
#include <stdio.h>


// Fatal, error, warn, info, debug
static const char *colourStrings[5] = {"30;41", "1;31", "1;33", "1;32", "100;30"};

void platform_consoleWrite(const char *message, const LogLevel colour) {
	printf("\033[%sm%s\033[0m", colourStrings[colour], message);
}

void platform_consoleWriteError(const char *message, const LogLevel colour) {
	printf("\033[%sm%s\033[0m", colourStrings[colour], message);
}

// Memory
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <string.h>


bool readFromMemory(void *handle, const uintptr_t address, size_t length, uint8_t *bytes) {
	mach_vm_size_t out = 0;
	const kern_return_t kr = mach_vm_read_overwrite(
		(task_t)(uintptr_t)handle,
		address,
		length,
		(mach_vm_address_t)bytes,
		&out
	);
	if (kr != KERN_SUCCESS || out != length) {
		memset(bytes, 0, length);
		platform_consoleWriteError("Failed to read memory\n", LogLevelError);
		return false;
	}
	return true;
}

uint8_t readByte(void *handle, const uintptr_t address) {
	mach_vm_size_t out = 0;
	uint8_t byte = 0;
	const kern_return_t kr = mach_vm_read_overwrite(
		(task_t)(uintptr_t)handle,
		address,
		1,
		(mach_vm_address_t)&byte,
		&out
	);
	if (kr != KERN_SUCCESS || out != 1) {
		platform_consoleWriteError("Failed to read byte\n", LogLevelError);
	}
	return byte;
}

void writeToMemory(void *handle, const uintptr_t address, const size_t length, const uint8_t *bytes) {
	const kern_return_t kr = mach_vm_write((task_t)(uintptr_t)handle, address, (vm_offset_t)bytes, length);
	if (kr != KERN_SUCCESS) {
		platform_consoleWriteError("Failed to write to memory\n", LogLevelError);
	}
}

// Process
#include <libproc.h>
#include <limits.h>
#include <stdlib.h>


// Scan the given task's virtual memory regions for a mapping backed by "fm.exe"
// and return the module base address (region start minus its file offset),
// mirroring the Linux /proc/<pid>/maps delta calculation. Returns 0 if not found.
static uintptr_t findModuleBase(const task_t task, const int32_t pid) {
	mach_vm_address_t address = 0;
	mach_vm_size_t size = 0;
	uint32_t depth = 0;

	while (true) {
		struct vm_region_submap_info_64 info;
		mach_msg_type_number_t count = VM_REGION_SUBMAP_INFO_COUNT_64;

		const kern_return_t kr = mach_vm_region_recurse(
			task,
			&address,
			&size,
			&depth,
			(vm_region_recurse_info_t)&info,
			&count
		);
		if (kr != KERN_SUCCESS) {
			break;
		}

		if (info.is_submap) {
			++depth;
			continue;
		}

		char path[PATH_MAX] = {0};
		if (proc_regionfilename(pid, address, path, sizeof(path)) > 0 && strstr(path, "fm.exe")) {
			return address - info.offset;
		}

		address += size;
	}

	return 0;
}

void platform_openProcess(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;
	context->moduleBaseAddress = 0;

	const int pidCapacity = proc_listpids(PROC_ALL_PIDS, 0, NULL, 0);
	if (pidCapacity <= 0) {
		platform_consoleWriteError("Could not enumerate processes\n", LogLevelError);
		return;
	}

	pid_t *pids = calloc((size_t)pidCapacity, sizeof(pid_t));
	if (!pids) {
		platform_consoleWriteError("Out of memory enumerating processes\n", LogLevelError);
		return;
	}

	const int bytesReturned = proc_listpids(PROC_ALL_PIDS, 0, pids, pidCapacity * (int)sizeof(pid_t));
	const int pidCount = bytesReturned > 0 ? bytesReturned / (int)sizeof(pid_t) : 0;

	for (int i = 0; i < pidCount; ++i) {
		const pid_t pid = pids[i];
		if (pid <= 0) {
			continue;
		}

		task_t task = MACH_PORT_NULL;
		if (task_for_pid(mach_task_self(), pid, &task) != KERN_SUCCESS) {
			// Not our process, or missing entitlement/privileges to inspect it.
			continue;
		}

		const uintptr_t baseAddr = findModuleBase(task, pid);
		if (baseAddr != 0) {
			context->handle = (void*)(uintptr_t)task;
			context->pid = (uint32_t)pid;
			context->moduleBaseAddress = baseAddr;
			break;
		}

		mach_port_deallocate(mach_task_self(), task);
	}

	free(pids);

	if (context->handle == NULL) {
		platform_consoleWriteError("Process 'fm.exe' not found\n", LogLevelError);
	}
}

// Time
int64_t platform_getMicroseconds(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (int64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// Paths
#include <mach-o/dyld.h>


void platform_getExecutableDirectory(char *buffer, const size_t size) {
	if (size == 0) {
		return;
	}

	uint32_t length = (uint32_t)size;
	if (_NSGetExecutablePath(buffer, &length) != 0) {
		platform_consoleWriteError("Could not resolve executable path\n", LogLevelError);
		buffer[0] = '\0';
		return;
	}

	buffer[size - 1] = '\0';

	// Strip the executable filename, leaving the directory.
	char *lastSlash = strrchr(buffer, '/');
	if (lastSlash) {
		lastSlash[1] = '\0';
	}
}
#endif
