// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <stdbool.h>

typedef struct FileWatcher FileWatcher;

typedef void (*FileWatcherCallback)(void *userData);

/**
 * Watches a single file for content changes, creation, replacement and atomic-save renames.
 *
 * Backed by GIO's file monitors, so the platform-native mechanism is used on each OS (inotify on
 * Linux, FSEvents on macOS, ReadDirectoryChangesW on Windows) with no polling.
 *
 * Bursts of events (editors truncate-then-write, or write-then-rename) are coalesced into a single
 * callback, which is always dispatched on the thread running the default main context.
 *
 * Returns NULL if a monitor could not be created.
 */
FileWatcher *fileWatcher_create(const char *path, FileWatcherCallback callback, void *userData);

void fileWatcher_destroy(FileWatcher *watcher);
