// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "platform.h"


// Logging
#include <stdio.h>
#include <string.h>
#include <windows.h>


static void writeToConsole(const char *message, const LogLevel colour, HANDLE consoleHandle) {
	if (consoleHandle == INVALID_HANDLE_VALUE) {
		fprintf(stderr, "%s", message);
		printf("\n");
		return;
	}

	// Fatal, error, warn, info, debug
	static uint8_t levels[5] = {64, 4, 6, 2, 8};
	SetConsoleTextAttribute(consoleHandle, levels[colour]);

	OutputDebugStringA(message);

	const DWORD length = (DWORD)strlen(message);
	WriteConsoleA(consoleHandle, message, length, NULL, NULL);
}

void platform_consoleWrite(const char *message, const LogLevel colour) {
	writeToConsole(message, colour, GetStdHandle(STD_OUTPUT_HANDLE));
}

void platform_consoleWriteError(const char *message, const LogLevel colour) {
	writeToConsole(message, colour, GetStdHandle(STD_ERROR_HANDLE));
}


// Memory
#include <stdint.h>


bool readFromMemory(void *handle, const uintptr_t address, const size_t length, uint8_t *bytes) {
	if (!ReadProcessMemory(handle, (LPCVOID)address, bytes, length, NULL)) {
		memset(bytes, 0, length);
		platform_consoleWriteError("Failed to read memory\n", LogLevelError);
		return false;
	}
	return true;
}

uint8_t readByte(void *handle, const uintptr_t address) {
	uint8_t byte = 0;
	if (!ReadProcessMemory(handle, (LPCVOID)address, &byte, 1, NULL)) {
		platform_consoleWriteError("Failed to read byte\n", LogLevelError);
	}
	return byte;
}

void writeToMemory(void *handle, const uintptr_t address, const size_t length, const uint8_t *bytes) {
	if (!WriteProcessMemory(handle, (LPVOID)address, bytes, length, NULL)) {
		platform_consoleWriteError("Failed to write to memory\n", LogLevelError);
	}
}


// Process
#include <stdlib.h>
#include <tlhelp32.h>

#include "core/logger.h"


void platform_openProcess(ProcessContext *context) {
	context->handle = NULL;
	context->pid = 0;

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		LOG_ERROR("Failed to create process snapshot: %ld", GetLastError());
		return;
	}

	PROCESSENTRY32W pe;
	pe.dwSize = sizeof(pe);


	DWORD pid = 0;
	{
		BOOL hResult = Process32FirstW(hSnapshot, &pe);
		while (hResult) {
			if (_wcsicmp(pe.szExeFile, L"fm.exe") == 0) {
				pid = pe.th32ProcessID;
				break;
			}
			hResult = Process32NextW(hSnapshot, &pe);
		}
	}

	CloseHandle(hSnapshot);

	if (pid == 0) {
		LOG_WARN("Process '%s' not found", "fm.exe");
		return;
	}

	HANDLE h = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (h == NULL || h == INVALID_HANDLE_VALUE) {
		LOG_ERROR("Failed to open process with PID %ld: access denied or process not found", pid);
		return;
	}

	// Get the base address of the module
	HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
	if (snap == INVALID_HANDLE_VALUE) {
		LOG_ERROR("Failed to create module snapshot: %ld", GetLastError());
		CloseHandle(h);
		return;
	}

	MODULEENTRY32W me;
	uintptr_t baseAddr = 0;
	bool moduleNotFound = true;
	me.dwSize = sizeof(MODULEENTRY32W);
	if (Module32FirstW(snap, &me)) {
		do {
			if (_wcsicmp(me.szModule, L"fm.exe") == 0) {
				baseAddr = (uintptr_t)me.modBaseAddr;
				moduleNotFound = false;
				break;
			}
		} while (Module32NextW(snap, &me));
	}

	CloseHandle(snap);

	if (moduleNotFound) {
		LOG_ERROR("Module 'fm.exe' not found in process with PID %ld", pid);
		CloseHandle(h);
		return;
	}

	context->handle = h;
	context->pid = pid;
	context->moduleBaseAddress = baseAddr;
}

// Time
void platform_closeProcess(ProcessContext *context) {
	if (context->handle) CloseHandle(context->handle);
	*context = (ProcessContext){0};
}

int64_t platform_getMicroseconds(void) {
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return count.QuadPart / freq.QuadPart * 1000000 + count.QuadPart % freq.QuadPart * 1000000 / freq.QuadPart;
}

// Paths
void platform_getExecutableDirectory(char *buffer, const size_t size) {
	if (size == 0) {
		return;
	}

	wchar_t wide[4096];
	const DWORD length = GetModuleFileNameW(NULL, wide, 4096);
	if (length == 0 || length >= 4096 ||
		!WideCharToMultiByte(CP_UTF8, 0, wide, -1, buffer, (int)size, NULL, NULL)) {
		platform_consoleWriteError("Could not resolve executable path\n", LogLevelError);
		buffer[0] = '\0';
		return;
	}

	// Strip the executable filename, leaving the directory (handle both separators).
	char *lastSlash = strrchr(buffer, '\\');
	char *lastForward = strrchr(buffer, '/');
	if (!lastSlash || (lastForward && lastForward > lastSlash)) {
		lastSlash = lastForward;
	}
	if (lastSlash) {
		lastSlash[1] = '\0';
	}
}
