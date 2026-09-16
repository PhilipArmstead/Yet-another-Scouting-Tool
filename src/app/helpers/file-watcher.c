// SPDX-FileCopyrightText: © 2026 Phil Armstead <philarmstead@mailbox.org>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "file-watcher.h"
#include "core/logger.h"

#include <gio/gio.h>


// Long enough to swallow the multi-event bursts editors produce when saving, short enough to feel
// immediate.
#define FILE_WATCHER_DEBOUNCE_MS 120

struct FileWatcher {
	GFileMonitor *monitor;
	FileWatcherCallback callback;
	void *userData;
	guint debounceId;
};


static gboolean onDebounceElapsed(gpointer userData) {
	FileWatcher *watcher = userData;
	watcher->debounceId = 0;
	watcher->callback(watcher->userData);

	return G_SOURCE_REMOVE;
}

static void onFileChanged(
	GFileMonitor *monitor,
	GFile *file,
	GFile *otherFile,
	const GFileMonitorEvent event,
	const gpointer userData
) {
	(void)monitor;
	(void)file;
	(void)otherFile;

	switch (event) {
		// A plain write finishing, or the file appearing/being replaced by an atomic save.
		case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
		case G_FILE_MONITOR_EVENT_CREATED:
		case G_FILE_MONITOR_EVENT_RENAMED:
		case G_FILE_MONITOR_EVENT_MOVED_IN:
			break;
		// Deletions leave nothing to read, and the replacement will arrive as its own event.
		default:
			return;
	}

	FileWatcher *watcher = userData;
	if (watcher->debounceId != 0) {
		g_source_remove(watcher->debounceId);
	}
	watcher->debounceId = g_timeout_add(FILE_WATCHER_DEBOUNCE_MS, onDebounceElapsed, watcher);
}

FileWatcher *fileWatcher_create(const char *path, const FileWatcherCallback callback, void *userData) {
	GFile *file = g_file_new_for_path(path);
	GError *error = NULL;
	GFileMonitor *monitor = g_file_monitor_file(file, G_FILE_MONITOR_WATCH_MOVES, NULL, &error);
	g_object_unref(file);

	if (!monitor) {
		LOG_WARN("Could not watch '%s' for changes: %s", path, error ? error->message : "unknown error");
		g_clear_error(&error);
		return NULL;
	}

	FileWatcher *watcher = g_new0(FileWatcher, 1);
	watcher->monitor = monitor;
	watcher->callback = callback;
	watcher->userData = userData;

	g_signal_connect(monitor, "changed", G_CALLBACK(onFileChanged), watcher);

	return watcher;
}

void fileWatcher_destroy(FileWatcher *watcher) {
	if (!watcher) {
		return;
	}

	if (watcher->debounceId != 0) {
		g_source_remove(watcher->debounceId);
	}

	g_file_monitor_cancel(watcher->monitor);
	g_object_unref(watcher->monitor);
	g_free(watcher);
}
