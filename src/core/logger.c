// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdarg.h>
#include <stdio.h>

#include "logger.h"
#include "platform/platform.h"


#define LOG_FILE_PATH "yast.log"

static FILE *logFile = NULL;

bool logger_init(void) {
	logFile = fopen(LOG_FILE_PATH, "a");
	if (logFile == NULL) {
		platform_consoleWriteError("[ERROR]: Failed to open log file: " LOG_FILE_PATH "\n", LogLevelError);
		return false;
	}

	return true;
}

void logger_shutdown(void) {
	if (logFile == NULL) {
		return;
	}

	fclose(logFile);
	logFile = NULL;
}

void logger_flush(void) {
	if (logFile != NULL) {
		fflush(logFile);
	}
}

#define PREFIX_SIZE 9  // "[FATAL]: " is the longest prefix
#define MAX_LOG_SIZE 4096
#define MAX_MESSAGE_SIZE (MAX_LOG_SIZE - PREFIX_SIZE - 2)  // -2 for \n and null terminator

void logger_output(LogLevel level, const char *message, ...) {
	const char *levelStrings[LogLevelCount] = {
		"[FATAL]: ",
		"[ERROR]: ",
		"[WARN]:  ",
		"[INFO]:  ",
		"[DEBUG]: "
	};

	if (level < 0 || level >= LogLevelCount) {
		// Invalid log level, default to info
		LOG_WARN("Invalid log level: %d", level);
		level = LogLevelInfo;
	}

	const bool isError = level < LogLevelWarn;
	char output[MAX_MESSAGE_SIZE] = {0};

	if (message == NULL) {
		message = "(null message)";
	}

	va_list args;
	va_start(args, message);
	vsnprintf(output, MAX_MESSAGE_SIZE, message, args);
	va_end(args);

	char outputWithPrefix[MAX_LOG_SIZE];
	snprintf(outputWithPrefix, MAX_LOG_SIZE, "%s%s\n", levelStrings[level], output);

	if (logFile != NULL) {
		fputs(outputWithPrefix, logFile);
		if (isError) {
			fflush(logFile);
		}
	}

	// Platform-specific output
	if (isError) {
		platform_consoleWriteError(outputWithPrefix, level);
	} else {
		platform_consoleWrite(outputWithPrefix, level);
	}
}
