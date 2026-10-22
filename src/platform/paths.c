#include "core/logger.h"
#include "platform.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef ARCH_WIN
#include <fcntl.h>
#include <io.h>
#include <shlobj.h>
#include <sys/stat.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

FILE *platform_fopen(const char *path, const char *mode) {
#ifdef ARCH_WIN
	wchar_t widePath[4096], wideMode[16];
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, widePath, 4096) ||
			!MultiByteToWideChar(CP_UTF8, 0, mode, -1, wideMode, 16)) {
		errno = EINVAL;
		return NULL;
	}
	if (mode[0] == 'w' && strchr(mode, 'x')) {
		const bool binary = strchr(mode, 'b') != NULL;
		const int descriptor = _wopen(widePath, _O_CREAT | _O_EXCL | _O_WRONLY | (binary ? _O_BINARY : _O_TEXT), _S_IREAD | _S_IWRITE);
		if (descriptor < 0)
			return NULL;
		FILE *file = _fdopen(descriptor, binary ? "wb" : "w");
		if (!file) {
			const int error = errno;
			_close(descriptor);
			errno = error;
		}
		return file;
	}
	return _wfopen(widePath, wideMode);
#else
	return fopen(path, mode);
#endif
}

bool platform_removeFile(const char *path) {
#ifdef ARCH_WIN
	wchar_t wide[4096];
	return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, 4096) && DeleteFileW(wide);
#else
	return remove(path) == 0;
#endif
}

bool platform_replaceFile(const char *source, const char *destination) {
#ifdef ARCH_WIN
	wchar_t from[4096], to[4096];
	return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, source, -1, from, 4096) &&
				 MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, destination, -1, to, 4096) &&
				 MoveFileExW(from, to, MOVEFILE_REPLACE_EXISTING);
#else
	return rename(source, destination) == 0;
#endif
}

static bool makeDirectory(const char *path) {
#ifdef ARCH_WIN
	wchar_t wide[4096];
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, 4096))
		return false;
	return CreateDirectoryW(wide, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
	return mkdir(path, 0700) == 0 || errno == EEXIST;
#endif
}

bool platform_getUserDirectory(char *buffer, const size_t size, const bool state) {
	char root[4096] = {0};
#ifdef ARCH_WIN
	PWSTR path = NULL;
	if (FAILED(SHGetKnownFolderPath(state ? &FOLDERID_LocalAppData : &FOLDERID_RoamingAppData, 0, NULL, &path))) {
		LOG_ERROR("Could not find the user application-data directory");
		return false;
	}
	const int converted = WideCharToMultiByte(CP_UTF8, 0, path, -1, root, sizeof(root), NULL, NULL);
	CoTaskMemFree(path);
	if (!converted)
		return false;
	const char *suffix = "/YaST/";
#else
	const char *home = getenv("HOME");
	if (!home || !*home) {
		LOG_ERROR("HOME is not set; cannot resolve user configuration");
		return false;
	}
#ifdef ARCH_MACOS
	snprintf(root, sizeof(root), "%s/Library", home);
	const char *suffix = state ? "/Logs/YaST/" : "/Application Support/YaST/";
#else
	const char *xdg = getenv(state ? "XDG_STATE_HOME" : "XDG_CONFIG_HOME");
	if (xdg && xdg[0] == '/')
		snprintf(root, sizeof(root), "%s", xdg);
	else
		snprintf(root, sizeof(root), "%s/%s", home, state ? ".local/state" : ".config");
	const char *suffix = "/yast/";
#endif
#endif
	const int length = snprintf(buffer, size, "%s%s", root, suffix);
	if (length < 0 || (size_t)length >= size) {
		LOG_ERROR("User configuration path exceeds buffer capacity");
		return false;
	}
	for (size_t i = 1; buffer[i]; ++i) {
		if (buffer[i] != '/' && buffer[i] != '\\')
			continue;
		const char saved = buffer[i];
		buffer[i] = '\0';
		const bool made = makeDirectory(buffer);
		buffer[i] = saved;
		if (!made) {
			LOG_ERROR("Could not create user directory '%s'", buffer);
			return false;
		}
	}
	return true;
}
