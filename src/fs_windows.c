#if !defined(CUSTOM_FS) && defined(_WIN32)

#include "c_utils/fs.h"
#include "c_utils/alloc.h"

#include <windows.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

struct File {
    HANDLE handle;
};

struct Directory {
    HANDLE handle;
    WIN32_FIND_DATAW data;
    bool first;
};

static FileResult fs_windows_error(DWORD error) {
    switch (error) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
    case ERROR_INVALID_NAME:
        return FS_NOT_FOUND;

    case ERROR_ACCESS_DENIED:
    case ERROR_PRIVILEGE_NOT_HELD:
        return FS_PERMISSION;

    case ERROR_ALREADY_EXISTS:
    case ERROR_FILE_EXISTS:
        return FS_EXISTS;

    case ERROR_DIRECTORY:
        return FS_NOT_DIRECTORY;

    case ERROR_DISK_FULL:
    case ERROR_HANDLE_DISK_FULL:
        return FS_NO_SPACE;

    case ERROR_READ_FAULT:
    case ERROR_WRITE_FAULT:
    case ERROR_CRC:
        return FS_IO_ERROR;

    default:
        return FS_ERROR;
    }
}

static wchar_t *fs_windows_utf8_to_wide(str8 path) {
    int length;
    wchar_t *result;

    if (path == NULL)
        return NULL;

    length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);

    if (length <= 0)
        return NULL;

    result = utils_malloc((size_t)length * sizeof(wchar_t));

    if (result == NULL)
        return NULL;

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, result, length) <= 0) {

        utils_free(result);
        return NULL;
    }

    return result;
}

static FileType fs_windows_entry_type(DWORD attributes) {
    /*
     * Directory junctions/symlinks have both DIRECTORY and REPARSE_POINT.
     * Keep them as directories so callers treat them as traversable paths.
     * File symlinks only have REPARSE_POINT.
     */
    if (attributes & FILE_ATTRIBUTE_DIRECTORY)
        return FS_DIRECTORY;

    if (attributes & FILE_ATTRIBUTE_REPARSE_POINT)
        return FS_SYMLINK;

    return FS_FILE;
}

FileResult fs_platform_open(str8 path, File **file) {
    wchar_t *wide_path;
    HANDLE handle;
    File *result;

    if (path == NULL || file == NULL)
        return FS_INVALID_PATH;

    *file = NULL;

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    handle =
        CreateFileW(wide_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    utils_free(wide_path);

    if (handle == INVALID_HANDLE_VALUE)
        return fs_windows_error(GetLastError());

    result = utils_malloc(sizeof(*result));

    if (result == NULL) {
        CloseHandle(handle);
        return FS_NO_SPACE;
    }

    result->handle = handle;

    *file = result;

    return FS_OK;
}

FileResult fs_platform_create(str8 path, File **file) {
    wchar_t *wide_path;
    HANDLE handle;
    File *result;

    if (path == NULL || file == NULL)
        return FS_INVALID_PATH;

    *file = NULL;

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    handle = CreateFileW(wide_path, GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                         CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    utils_free(wide_path);

    if (handle == INVALID_HANDLE_VALUE)
        return fs_windows_error(GetLastError());

    result = utils_malloc(sizeof(*result));

    if (result == NULL) {
        CloseHandle(handle);
        return FS_NO_SPACE;
    }

    result->handle = handle;

    *file = result;

    return FS_OK;
}

FileResult fs_platform_close(File *file) {
    BOOL result;

    if (file == NULL)
        return FS_ERROR;

    result = CloseHandle(file->handle);

    utils_free(file);

    if (!result)
        return fs_windows_error(GetLastError());

    return FS_OK;
}

FileResult fs_platform_read(File *file, void *buffer, usize size, usize *bytes_read) {
    DWORD requested;
    DWORD result;

    if (file == NULL || buffer == NULL)
        return FS_ERROR;

    if (bytes_read != NULL)
        *bytes_read = 0;

    if (size == 0)
        return FS_OK;

    requested = size > (usize)MAXDWORD ? MAXDWORD : (DWORD)size;

    if (!ReadFile(file->handle, buffer, requested, &result, NULL)) {

        return fs_windows_error(GetLastError());
    }

    if (bytes_read != NULL)
        *bytes_read = (usize)result;

    return FS_OK;
}

FileResult fs_platform_write(File *file, const void *buffer, usize size, usize *bytes_written) {
    DWORD requested;
    DWORD result;

    if (file == NULL || buffer == NULL)
        return FS_ERROR;

    if (bytes_written != NULL)
        *bytes_written = 0;

    if (size == 0)
        return FS_OK;

    requested = size > (usize)MAXDWORD ? MAXDWORD : (DWORD)size;

    if (!WriteFile(file->handle, buffer, requested, &result, NULL)) {

        return fs_windows_error(GetLastError());
    }

    if (bytes_written != NULL)
        *bytes_written = (usize)result;

    return FS_OK;
}

FileResult fs_platform_seek(File *file, u64 position) {
    LARGE_INTEGER offset;
    LARGE_INTEGER result;

    if (file == NULL)
        return FS_ERROR;

    if (position > (u64)INT64_MAX)
        return FS_ERROR;

    offset.QuadPart = (LONGLONG)position;

    if (!SetFilePointerEx(file->handle, offset, &result, FILE_BEGIN)) {

        return fs_windows_error(GetLastError());
    }

    return FS_OK;
}

FileResult fs_platform_tell(File *file, u64 *position) {
    LARGE_INTEGER zero;
    LARGE_INTEGER result;

    if (file == NULL || position == NULL)
        return FS_ERROR;

    zero.QuadPart = 0;

    if (!SetFilePointerEx(file->handle, zero, &result, FILE_CURRENT)) {

        return fs_windows_error(GetLastError());
    }

    *position = (u64)result.QuadPart;

    return FS_OK;
}

FileResult fs_platform_stat(str8 path, FileInfo *info) {
    wchar_t *wide_path;
    WIN32_FILE_ATTRIBUTE_DATA data;
    ULARGE_INTEGER size;

    if (path == NULL || info == NULL)
        return FS_ERROR;

    memset(info, 0, sizeof(*info));

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    if (!GetFileAttributesExW(wide_path, GetFileExInfoStandard, &data)) {

        DWORD error = GetLastError();

        utils_free(wide_path);

        return fs_windows_error(error);
    }

    utils_free(wide_path);

    info->type = fs_windows_entry_type(data.dwFileAttributes);

    size.LowPart = data.nFileSizeLow;
    size.HighPart = data.nFileSizeHigh;

    info->size = (u64)size.QuadPart;

    return FS_OK;
}

FileResult fs_platform_exists(str8 path) {
    wchar_t *wide_path;
    DWORD attributes;

    if (path == NULL)
        return FS_INVALID_PATH;

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    attributes = GetFileAttributesW(wide_path);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();

        utils_free(wide_path);

        return fs_windows_error(error);
    }

    utils_free(wide_path);

    return FS_OK;
}

FileResult fs_platform_remove(str8 path) {
    wchar_t *wide_path;
    DWORD attributes;

    if (path == NULL)
        return FS_INVALID_PATH;

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    attributes = GetFileAttributesW(wide_path);

    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();

        utils_free(wide_path);

        return fs_windows_error(error);
    }

    if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!RemoveDirectoryW(wide_path)) {
            DWORD error = GetLastError();

            utils_free(wide_path);

            return fs_windows_error(error);
        }
    } else {
        if (!DeleteFileW(wide_path)) {
            DWORD error = GetLastError();

            utils_free(wide_path);

            return fs_windows_error(error);
        }
    }

    utils_free(wide_path);

    return FS_OK;
}

FileResult fs_platform_rename(str8 old_path, str8 new_path) {
    wchar_t *old_wide;
    wchar_t *new_wide;

    if (old_path == NULL || new_path == NULL)
        return FS_INVALID_PATH;

    old_wide = fs_windows_utf8_to_wide(old_path);

    if (old_wide == NULL)
        return FS_INVALID_PATH;

    new_wide = fs_windows_utf8_to_wide(new_path);

    if (new_wide == NULL) {
        utils_free(old_wide);
        return FS_INVALID_PATH;
    }

    if (!MoveFileExW(old_wide, new_wide, MOVEFILE_REPLACE_EXISTING)) {

        DWORD error = GetLastError();

        utils_free(old_wide);
        utils_free(new_wide);

        return fs_windows_error(error);
    }

    utils_free(old_wide);
    utils_free(new_wide);

    return FS_OK;
}

FileResult fs_platform_mkdir(str8 path) {
    wchar_t *wide_path;

    if (path == NULL)
        return FS_INVALID_PATH;

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    if (!CreateDirectoryW(wide_path, NULL)) {
        DWORD error = GetLastError();

        utils_free(wide_path);

        return fs_windows_error(error);
    }

    utils_free(wide_path);

    return FS_OK;
}

FileResult fs_platform_opendir(str8 path, Directory **directory) {
    wchar_t *wide_path;
    wchar_t *pattern;
    size_t length;
    HANDLE handle;
    Directory *result;

    if (path == NULL || directory == NULL)
        return FS_INVALID_PATH;

    *directory = NULL;

    wide_path = fs_windows_utf8_to_wide(path);

    if (wide_path == NULL)
        return FS_INVALID_PATH;

    length = wcslen(wide_path);

    pattern = utils_malloc((length + 3) * sizeof(wchar_t));

    if (pattern == NULL) {
        utils_free(wide_path);
        return FS_NO_SPACE;
    }

    wcscpy(pattern, wide_path);

    if (length > 0 && pattern[length - 1] != L'\\' && pattern[length - 1] != L'/') {

        pattern[length++] = L'\\';
    }

    pattern[length++] = L'*';
    pattern[length] = L'\0';

    utils_free(wide_path);

    result = utils_malloc(sizeof(*result));

    if (result == NULL) {
        utils_free(pattern);
        return FS_NO_SPACE;
    }

    handle = FindFirstFileW(pattern, &result->data);

    utils_free(pattern);

    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();

        utils_free(result);

        return fs_windows_error(error);
    }

    result->handle = handle;
    result->first = true;

    *directory = result;

    return FS_OK;
}

FileResult fs_platform_readdir(Directory *directory, DirectoryEntry *entry) {
    WIN32_FIND_DATAW *data;

    if (directory == NULL || entry == NULL)
        return FS_ERROR;

    data = &directory->data;

    for (;;) {
        BOOL valid;

        if (directory->first) {
            directory->first = false;
            valid = TRUE;
        } else {
            valid = FindNextFileW(directory->handle, &directory->data);
        }

        if (!valid) {
            DWORD error = GetLastError();

            if (error == ERROR_NO_MORE_FILES)
                return FS_NOT_FOUND;

            return fs_windows_error(error);
        }

        if (wcscmp(data->cFileName, L".") == 0 || wcscmp(data->cFileName, L"..") == 0) {
            continue;
        }

        memset(entry, 0, sizeof(*entry));

        if (!WideCharToMultiByte(CP_UTF8, 0, data->cFileName, -1, entry->name, FS_MAX_NAME, NULL,
                                 NULL)) {

            return FS_ERROR;
        }

        entry->type = fs_windows_entry_type(data->dwFileAttributes);

        return FS_OK;
    }
}

FileResult fs_platform_closedir(Directory *directory) {
    BOOL result;

    if (directory == NULL)
        return FS_ERROR;

    result = FindClose(directory->handle);

    utils_free(directory);

    if (!result)
        return fs_windows_error(GetLastError());

    return FS_OK;
}

#endif /* !CUSTOM_FS && _WIN32 */

#if defined(CUSTOM_FS) || !defined(_WIN32)
/* Avoid -Wpedantic empty translation unit. */
typedef int fs_windows_unused;
#endif
