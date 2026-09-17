// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "app/types.h"

#include <stdlib.h>


bool logger_init(void);
void logger_shutdown(void);
void logger_flush(void);
void logger_output(LogLevel level, const char *message, ...) G_GNUC_PRINTF(2, 3);

#define ERROR_EXIT(...) { LOG_FATAL(__VA_ARGS__); exit(1); }
#define ERROR_RETURN(R, ...) { LOG_FATAL(__VA_ARGS__); return (R); }
#define LOG_FATAL(...) logger_output(LogLevelFatal, __VA_ARGS__)
#define LOG_ERROR(...) logger_output(LogLevelError, __VA_ARGS__)
#define LOG_WARN(...) logger_output(LogLevelWarn, __VA_ARGS__)
#define LOG_INFO(...) logger_output(LogLevelInfo, __VA_ARGS__)
#ifdef DEBUG
#define LOG_DEBUG(...) logger_output(LogLevelDebug, __VA_ARGS__)
#else
#define LOG_DEBUG(...) do { } while (0)
#endif
