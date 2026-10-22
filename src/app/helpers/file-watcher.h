#pragma once
#include <stdbool.h>

typedef struct FileWatcher FileWatcher;
typedef void (*FileWatcherCallback)(void *userData);

FileWatcher *fileWatcher_create(const char *path, FileWatcherCallback callback, void *userData);
bool fileWatcher_poll(FileWatcher *watcher);
void fileWatcher_destroy(FileWatcher *watcher);
