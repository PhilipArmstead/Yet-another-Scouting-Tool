#include "file-watcher.h"
#include "core/logger.h"
#include "platform/platform.h"
#include <stdlib.h>
#include <string.h>
#ifdef ARCH_WIN
#include <windows.h>
#else
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef ARCH_MACOS
#include <sys/event.h>
#else
#include <sys/inotify.h>
#endif
#endif

struct FileWatcher {
	FileWatcherCallback callback;
	void *userData;
	char path[4096], name[512];
	int64_t deadline;
	bool failed;
#ifdef ARCH_WIN
	bool pending;
	HANDLE directory;
	OVERLAPPED overlapped;
	_Alignas(DWORD) unsigned char events[16384];
	wchar_t wideName[512];
#else
	int descriptor;
#ifdef ARCH_MACOS
	int directory, file;
	bool initialized;
#endif
#endif
};

#ifdef ARCH_WIN
static bool arm(FileWatcher *watcher) {
	ResetEvent(watcher->overlapped.hEvent);
	watcher->pending = ReadDirectoryChangesW(watcher->directory, watcher->events, sizeof(watcher->events), FALSE,
																					 FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE,
																					 NULL, &watcher->overlapped, NULL) != 0;
	return watcher->pending;
}
#endif

FileWatcher *fileWatcher_create(const char *path, FileWatcherCallback callback, void *userData) {
	FileWatcher *watcher = calloc(1, sizeof(*watcher));
	if (!watcher) {
		LOG_ERROR("Could not allocate file watcher");
		return NULL;
	}
#ifndef ARCH_WIN
	watcher->descriptor = -1;
#ifdef ARCH_MACOS
	watcher->directory = -1;
	watcher->file = -1;
#endif
#endif
	char directory[4096];
	const int length = snprintf(directory, sizeof(directory), "%s", path);
	if (length < 0 || (size_t)length >= sizeof(directory))
		goto fail;
	char *slash = strrchr(directory, '/');
#ifdef ARCH_WIN
	char *backslash = strrchr(directory, '\\');
	if (!slash || (backslash && backslash > slash))
		slash = backslash;
#endif
	if (!slash || strlen(slash + 1) >= sizeof(watcher->name))
		goto fail;
	snprintf(watcher->path, sizeof(watcher->path), "%s", path);
	snprintf(watcher->name, sizeof(watcher->name), "%s", slash + 1);
	*slash = '\0';
	watcher->callback = callback;
	watcher->userData = userData;
#ifdef ARCH_WIN
	wchar_t wide[4096];
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, directory, -1, wide, 4096) ||
			!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, watcher->name, -1, watcher->wideName, 512))
		goto fail;
	watcher->directory = CreateFileW(wide, FILE_LIST_DIRECTORY,
																	 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
																	 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
	if (watcher->directory == INVALID_HANDLE_VALUE)
		goto fail;
	watcher->overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
	if (!watcher->overlapped.hEvent || !arm(watcher))
		goto fail;
#elif defined(ARCH_MACOS)
	watcher->descriptor = -1;
	watcher->directory = -1;
	watcher->file = -1;
	watcher->descriptor = kqueue();
	watcher->directory = open(directory[0] ? directory : "/", O_EVTONLY | O_CLOEXEC);
	if (watcher->descriptor < 0 || watcher->directory < 0)
		goto fail;
	struct kevent event;
	EV_SET(&event, (uintptr_t)watcher->directory, EVFILT_VNODE, EV_ADD | EV_CLEAR,
				 NOTE_WRITE | NOTE_RENAME | NOTE_DELETE, 0, NULL);
	if (kevent(watcher->descriptor, &event, 1, NULL, 0, NULL) < 0)
		goto fail;
#else
	watcher->descriptor = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	if (watcher->descriptor < 0 ||
			inotify_add_watch(watcher->descriptor, directory[0] ? directory : "/",
												IN_CLOSE_WRITE | IN_MOVED_TO | IN_CREATE | IN_DELETE | IN_DELETE_SELF | IN_MOVE_SELF) < 0)
		goto fail;
#endif
#ifdef ARCH_MACOS
	if (!fileWatcher_poll(watcher))
		goto fail;
#endif
	return watcher;
fail:
	LOG_ERROR("Could not watch '%s' for changes", path);
	fileWatcher_destroy(watcher);
	return NULL;
}

bool fileWatcher_poll(FileWatcher *watcher) {
	if (!watcher || watcher->failed)
		return false;
	bool changed = false;
#ifdef ARCH_WIN
	DWORD bytes = 0;
	if (GetOverlappedResult(watcher->directory, &watcher->overlapped, &bytes, FALSE)) {
		watcher->pending = false;
		if (bytes == 0)
			changed = true;
		for (DWORD offset = 0; offset < bytes;) {
			FILE_NOTIFY_INFORMATION *event = (void *)(watcher->events + offset);
			if (event->FileNameLength == wcslen(watcher->wideName) * sizeof(wchar_t) &&
					CompareStringOrdinal(event->FileName, (int)(event->FileNameLength / sizeof(wchar_t)),
															 watcher->wideName, (int)wcslen(watcher->wideName), TRUE) == CSTR_EQUAL)
				changed = true;
			if (!event->NextEntryOffset)
				break;
			offset += event->NextEntryOffset;
		}
		if (!arm(watcher)) {
			LOG_ERROR("Could not rearm options file notifications (%lu)", GetLastError());
			watcher->failed = true;
		}
	} else if (GetLastError() != ERROR_IO_INCOMPLETE) {
		LOG_ERROR("File notification read failed (%lu)", GetLastError());
		watcher->failed = true;
	}
#elif defined(ARCH_MACOS)
	if (watcher->file < 0) {
		watcher->file = open(watcher->path, O_EVTONLY | O_CLOEXEC);
		if (watcher->file >= 0) {
			changed = watcher->initialized;
			struct kevent event;
			EV_SET(&event, (uintptr_t)watcher->file, EVFILT_VNODE, EV_ADD | EV_CLEAR,
						 NOTE_WRITE | NOTE_EXTEND | NOTE_RENAME | NOTE_DELETE, 0, NULL);
			if (kevent(watcher->descriptor, &event, 1, NULL, 0, NULL) < 0) {
				LOG_ERROR("Could not watch options file contents: %s", strerror(errno));
				watcher->failed = true;
			}
		} else if (errno != ENOENT) {
			LOG_ERROR("Could not reopen options file notifications: %s", strerror(errno));
			watcher->failed = true;
		}
	}
	watcher->initialized = true;
	struct kevent events[8];
	const struct timespec zero = {0};
	const int count = kevent(watcher->descriptor, NULL, 0, events, 8, &zero);
	if (count < 0 && errno != EINTR) {
		LOG_ERROR("File notification read failed: %s", strerror(errno));
		watcher->failed = true;
	}
	for (int i = 0; i < count; ++i) {
		bool replaced = false;
		if (events[i].ident == (uintptr_t)watcher->directory) {
			if (events[i].fflags & (NOTE_RENAME | NOTE_DELETE)) {
				LOG_ERROR("Options directory moved or deleted; restart to restore hot reload");
				watcher->failed = true;
			}
			struct stat current, previous;
			const bool exists = stat(watcher->path, &current) == 0;
			replaced = watcher->file < 0 ? exists : !exists || fstat(watcher->file, &previous) != 0 || current.st_ino != previous.st_ino || current.st_dev != previous.st_dev;
		} else {
			changed = true;
			replaced = (events[i].fflags & (NOTE_DELETE | NOTE_RENAME)) != 0;
		}
		if (replaced) {
			changed = true;
			if (watcher->file >= 0)
				close(watcher->file);
			watcher->file = -1;
		}
	}
#else
	_Alignas(struct inotify_event) char events[16384];
	ssize_t count;
	while ((count = read(watcher->descriptor, events, sizeof(events))) > 0) {
		for (size_t offset = 0; offset < (size_t)count;) {
			const struct inotify_event *event = (const void *)(events + offset);
			if (event->mask & (IN_IGNORED | IN_DELETE_SELF | IN_MOVE_SELF)) {
				LOG_ERROR("Options directory notifications stopped; restart to restore hot reload");
				watcher->failed = true;
			}
			if ((event->mask & IN_Q_OVERFLOW) || (event->len && !strcmp(event->name, watcher->name)))
				changed = true;
			offset += sizeof(*event) + event->len;
		}
	}
	if (count < 0 && errno != EAGAIN && errno != EINTR) {
		LOG_ERROR("File notification read failed: %s", strerror(errno));
		watcher->failed = true;
	}
#endif
	const int64_t now = platform_getMicroseconds();
	if (changed)
		watcher->deadline = now + 120000;
	if (watcher->deadline && now >= watcher->deadline) {
		watcher->deadline = 0;
		watcher->callback(watcher->userData);
	}
	return !watcher->failed;
}

void fileWatcher_destroy(FileWatcher *watcher) {
	if (!watcher)
		return;
#ifdef ARCH_WIN
	if (watcher->directory && watcher->directory != INVALID_HANDLE_VALUE) {
		if (watcher->pending) {
			CancelIoEx(watcher->directory, &watcher->overlapped);
			DWORD transferred;
			GetOverlappedResult(watcher->directory, &watcher->overlapped, &transferred, TRUE);
		}
		CloseHandle(watcher->directory);
	}
	if (watcher->overlapped.hEvent)
		CloseHandle(watcher->overlapped.hEvent);
#else
	if (watcher->descriptor >= 0)
		close(watcher->descriptor);
#ifdef ARCH_MACOS
	if (watcher->file >= 0)
		close(watcher->file);
	if (watcher->directory >= 0)
		close(watcher->directory);
#endif
#endif
	free(watcher);
}
