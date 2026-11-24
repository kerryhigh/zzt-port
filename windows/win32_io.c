#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "win32_io.h"
#include <windows.h>

static uint8_t file_getc(zoo_io_handle *handle) {
	int value = fgetc((FILE *) handle->p);
	return value == EOF ? 0 : (uint8_t) value;
}

static size_t file_putc(zoo_io_handle *handle, uint8_t value) {
	return fputc(value, (FILE *) handle->p) == value ? 1 : 0;
}

static size_t file_read(zoo_io_handle *handle, uint8_t *dest, size_t len) {
	return fread(dest, 1, len, (FILE *) handle->p);
}

static size_t file_write(zoo_io_handle *handle, const uint8_t *src, size_t len) {
	return fwrite(src, 1, len, (FILE *) handle->p);
}

static size_t file_skip(zoo_io_handle *handle, size_t len) {
	FILE *file = (FILE *) handle->p;
	if (len > LONG_MAX || fseek(file, (long) len, SEEK_CUR) != 0) return 0;
	return len;
}

static size_t file_tell(zoo_io_handle *handle) {
	long position = ftell((FILE *) handle->p);
	return position < 0 ? 0 : (size_t) position;
}

static void file_close(zoo_io_handle *handle) {
	if (handle->p != NULL) fclose((FILE *) handle->p);
	handle->p = NULL;
}

static zoo_io_handle open_absolute(zoo_io_path_driver *driver, const char *name, zoo_io_mode mode) {
	FILE *file;
	zoo_io_handle handle;
	(void) driver;

	file = fopen(name, mode == MODE_WRITE ? "wb" : "rb");
	if (file == NULL) return zoo_io_open_file_empty();
	memset(&handle, 0, sizeof(handle));
	handle.p = file;
	handle.func_getc = file_getc;
	handle.func_putc = file_putc;
	handle.func_read = file_read;
	handle.func_write = file_write;
	handle.func_skip = file_skip;
	handle.func_tell = file_tell;
	handle.func_close = file_close;
	return handle;
}

static int64_t file_time_to_unix(const FILETIME *file_time) {
	ULARGE_INTEGER ticks;
	ticks.LowPart = file_time->dwLowDateTime;
	ticks.HighPart = file_time->dwHighDateTime;
	return ticks.QuadPart < 116444736000000000ULL
		? 0
		: (int64_t) ((ticks.QuadPart - 116444736000000000ULL) / 10000000ULL);
}

static bool scan_directory(zoo_io_path_driver *driver, const char *path,
		zoo_func_io_scan_dir_callback callback, void *callback_arg) {
	WIN32_FIND_DATAA data;
	HANDLE find_handle;
	char pattern[ZOO_PATH_MAX + 5];
	char *cursor;
	size_t path_len = strlen(path);
	zoo_io_dirent entry;
	bool keep_going = true;
	(void) driver;

	if (path_len + 3 >= sizeof(pattern)) return false;
	memcpy(pattern, path, path_len + 1);
	cursor = pattern + path_len;
	if (path_len > 0 && path[path_len - 1] != '\\' && path[path_len - 1] != '/') {
		*cursor++ = '\\';
	}
	memcpy(cursor, "*", 2);
	find_handle = FindFirstFileA(pattern, &data);
	if (find_handle == INVALID_HANDLE_VALUE) return false;

	do {
		strncpy(entry.name, data.cFileName, ZOO_PATH_MAX);
		entry.name[ZOO_PATH_MAX] = '\0';
		entry.type = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TYPE_DIR : TYPE_FILE;
		entry.mtime = entry.type == TYPE_FILE ? file_time_to_unix(&data.ftLastWriteTime) : 0;
		keep_going = callback(driver, &entry, callback_arg);
	} while (keep_going && FindNextFileA(find_handle, &data));

	FindClose(find_handle);
	return true;
}

void zoo_io_create_win32_driver(zoo_io_path_driver *driver) {
	DWORD length;
	zoo_io_internal_init_path_driver(driver);
	length = GetCurrentDirectoryA(ZOO_PATH_MAX, driver->path);
	if (length == 0 || length >= ZOO_PATH_MAX) {
		strncpy(driver->path, ".", ZOO_PATH_MAX);
		driver->path[ZOO_PATH_MAX] = '\0';
	}
	driver->func_open_file_absolute = open_absolute;
	driver->func_dir_scan = scan_directory;
}
