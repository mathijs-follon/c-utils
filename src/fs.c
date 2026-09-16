#include "fs.h"
#include "alloc.h"
#include "fs_platform.h"
#include "str.h"

#include <stdlib.h>
#include <string.h>

FileResult fs_open(StringView path, File **file) {
    str8 cpath;
    FileResult result;

    if (file == NULL)
        return FS_ERROR;

    *file = NULL;

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return FS_INVALID_PATH;

    result = fs_platform_open(cpath, file);

    utils_free(cpath);

    return result;
}

FileResult fs_create(StringView path, File **file) {
    str8 cpath;
    FileResult result;

    if (file == NULL)
        return FS_ERROR;

    *file = NULL;

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return FS_INVALID_PATH;

    result = fs_platform_create(cpath, file);

    utils_free(cpath);

    return result;
}

FileResult fs_close(File *file) {
    if (file == NULL)
        return FS_ERROR;

    return fs_platform_close(file);
}

FileResult fs_read(File *file, void *buffer, usize size, usize *bytes_read) {
    if (file == NULL || buffer == NULL)
        return FS_ERROR;

    if (bytes_read != NULL)
        *bytes_read = 0;

    return fs_platform_read(file, buffer, size, bytes_read);
}

FileResult fs_write(File *file, const void *buffer, usize size, usize *bytes_written) {
    if (file == NULL || buffer == NULL)
        return FS_ERROR;

    if (bytes_written != NULL)
        *bytes_written = 0;

    return fs_platform_write(file, buffer, size, bytes_written);
}

/* -------------------------------------------------------------------------- */
/* File position                                                              */
/* -------------------------------------------------------------------------- */

FileResult fs_seek(File *file, u64 position) {
    if (file == NULL)
        return FS_ERROR;

    return fs_platform_seek(file, position);
}

FileResult fs_tell(File *file, u64 *position) {
    if (file == NULL || position == NULL)
        return FS_ERROR;

    return fs_platform_tell(file, position);
}

FileResult fs_read_all(StringView path, void **data, usize *size) {
    FileResult result;
    FileInfo info;
    File *file = NULL;
    void *buffer = NULL;
    usize bytes_read = 0;
    usize total = 0;
    usize capacity = 0;

    if (data == NULL || size == NULL)
        return FS_ERROR;

    *data = NULL;
    *size = 0;

    result = fs_stat(path, &info);

    if (result != FS_OK)
        return result;

    if (info.type == FS_DIRECTORY)
        return FS_IS_DIRECTORY;

    result = fs_open(path, &file);

    if (result != FS_OK)
        return result;

    if (info.type == FS_FILE) {
        if (info.size == 0) {
            fs_close(file);
            return FS_OK;
        }

        if (info.size > (u64)SIZE_MAX) {
            fs_close(file);
            return FS_ERROR;
        }

        buffer = utils_malloc((size_t)info.size);

        if (buffer == NULL) {
            fs_close(file);
            return FS_NO_SPACE;
        }

        result = fs_read(file, buffer, (usize)info.size, &bytes_read);
        fs_close(file);

        if (result != FS_OK) {
            utils_free(buffer);
            return result;
        }

        if (bytes_read != (usize)info.size) {
            utils_free(buffer);
            return FS_IO_ERROR;
        }

        *data = buffer;
        *size = bytes_read;
        return FS_OK;
    }

    capacity = 4096;
    buffer = utils_malloc(capacity);

    if (buffer == NULL) {
        fs_close(file);
        return FS_NO_SPACE;
    }

    for (;;) {
        usize chunk = 0;

        if (total == capacity) {
            usize new_capacity;
            void *resized;

            if (capacity > SIZE_MAX / 2) {
                utils_free(buffer);
                fs_close(file);
                return FS_NO_SPACE;
            }

            new_capacity = capacity * 2;
            resized = utils_realloc(buffer, new_capacity);

            if (resized == NULL) {
                utils_free(buffer);
                fs_close(file);
                return FS_NO_SPACE;
            }

            buffer = resized;
            capacity = new_capacity;
        }

        result = fs_read(file, (char *)buffer + total, capacity - total, &chunk);

        if (result != FS_OK) {
            utils_free(buffer);
            fs_close(file);
            return result;
        }

        if (chunk == 0)
            break;

        total += chunk;
    }

    fs_close(file);

    if (total == 0) {
        utils_free(buffer);
        return FS_OK;
    }

    *data = buffer;
    *size = total;
    return FS_OK;
}

FileResult fs_write_all(StringView path, const void *data, usize size) {
    FileResult result;
    File *file = NULL;
    usize bytes_written = 0;

    if (data == NULL && size != 0)
        return FS_ERROR;

    result = fs_create(path, &file);

    if (result != FS_OK)
        return result;

    if (size == 0) {
        fs_close(file);
        return FS_OK;
    }

    result = fs_write(file, data, size, &bytes_written);

    fs_close(file);

    if (result != FS_OK)
        return result;

    if (bytes_written != size)
        return FS_IO_ERROR;

    return FS_OK;
}

FileResult fs_remove(StringView path) {
    str8 cpath;
    FileResult result;

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return FS_INVALID_PATH;

    result = fs_platform_remove(cpath);

    utils_free(cpath);

    return result;
}

FileResult fs_rename(StringView old_path, StringView new_path) {
    str8 old_cpath;
    str8 new_cpath;
    FileResult result;

    old_cpath = string_view_to_cstr(old_path);

    if (old_cpath == NULL)
        return FS_INVALID_PATH;

    new_cpath = string_view_to_cstr(new_path);

    if (new_cpath == NULL) {
        utils_free(old_cpath);
        return FS_INVALID_PATH;
    }

    result = fs_platform_rename(old_cpath, new_cpath);

    utils_free(old_cpath);
    utils_free(new_cpath);

    return result;
}

FileResult fs_stat(StringView path, FileInfo *info) {
    str8 cpath;
    FileResult result;

    if (info == NULL)
        return FS_ERROR;

    memset(info, 0, sizeof(*info));

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return FS_INVALID_PATH;

    result = fs_platform_stat(cpath, info);

    utils_free(cpath);

    return result;
}

bool fs_exists(StringView path) {
    str8 cpath;
    FileResult result;

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return false;

    result = fs_platform_exists(cpath);

    utils_free(cpath);

    return result == FS_OK;
}

FileResult fs_mkdir(StringView path) {
    str8 cpath;
    FileResult result;

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return FS_INVALID_PATH;

    result = fs_platform_mkdir(cpath);

    utils_free(cpath);

    return result;
}

FileResult fs_opendir(StringView path, Directory **directory) {
    str8 cpath;
    FileResult result;

    if (directory == NULL)
        return FS_ERROR;

    *directory = NULL;

    cpath = string_view_to_cstr(path);

    if (cpath == NULL)
        return FS_INVALID_PATH;

    result = fs_platform_opendir(cpath, directory);

    utils_free(cpath);

    return result;
}

FileResult fs_readdir(Directory *directory, DirectoryEntry *entry) {
    if (directory == NULL || entry == NULL)
        return FS_ERROR;

    memset(entry, 0, sizeof(*entry));

    return fs_platform_readdir(directory, entry);
}

FileResult fs_closedir(Directory *directory) {
    if (directory == NULL)
        return FS_ERROR;

    return fs_platform_closedir(directory);
}

StringView fs_strerror(FileResult result) {
    switch (result) {
    case FS_OK:
        return CSTR_TO_VIEW("success");

    case FS_ERROR:
        return CSTR_TO_VIEW("filesystem error");

    case FS_NOT_FOUND:
        return CSTR_TO_VIEW("file or directory not found");

    case FS_PERMISSION:
        return CSTR_TO_VIEW("permission denied");

    case FS_EXISTS:
        return CSTR_TO_VIEW("file or directory already exists");

    case FS_INVALID_PATH:
        return CSTR_TO_VIEW("invalid path");

    case FS_NOT_DIRECTORY:
        return CSTR_TO_VIEW("not a directory");

    case FS_IS_DIRECTORY:
        return CSTR_TO_VIEW("is a directory");

    case FS_NO_SPACE:
        return CSTR_TO_VIEW("not enough memory or disk space");

    case FS_IO_ERROR:
        return CSTR_TO_VIEW("I/O error");

    default:
        return CSTR_TO_VIEW("unknown filesystem error");
    }
}
