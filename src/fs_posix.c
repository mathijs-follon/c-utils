#if !defined(CUSTOM_FS) && !defined(_WIN32)

#include "c_utils/fs.h"
#include "c_utils/alloc.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct File {
    int fd;
};

struct Directory {
    DIR *dir;
};

static FileResult fs_posix_error(int error) {
    switch (error) {
    case ENOENT:
        return FS_NOT_FOUND;

    case EACCES:
    case EPERM:
        return FS_PERMISSION;

    case EEXIST:
        return FS_EXISTS;

    case ENOTDIR:
        return FS_NOT_DIRECTORY;

    case EISDIR:
        return FS_IS_DIRECTORY;

    case ENOSPC:
        return FS_NO_SPACE;

    case EIO:
        return FS_IO_ERROR;

    default:
        return FS_ERROR;
    }
}

FileResult fs_platform_open(str8 path, File **file) {
    File *result;
    int fd;

    if (path == NULL || file == NULL)
        return FS_INVALID_PATH;

    *file = NULL;

    fd = open(path, O_RDONLY);

    if (fd == -1)
        return fs_posix_error(errno);

    result = utils_malloc(sizeof(*result));

    if (result == NULL) {
        close(fd);
        return FS_NO_SPACE;
    }

    result->fd = fd;

    *file = result;

    return FS_OK;
}

FileResult fs_platform_create(str8 path, File **file) {
    File *result;
    int fd;

    if (path == NULL || file == NULL)
        return FS_INVALID_PATH;

    *file = NULL;

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

    if (fd == -1)
        return fs_posix_error(errno);

    result = utils_malloc(sizeof(*result));

    if (result == NULL) {
        close(fd);
        return FS_NO_SPACE;
    }

    result->fd = fd;

    *file = result;

    return FS_OK;
}

FileResult fs_platform_close(File *file) {
    int result;

    if (file == NULL)
        return FS_ERROR;

    result = close(file->fd);

    utils_free(file);

    if (result == -1)
        return fs_posix_error(errno);

    return FS_OK;
}

FileResult fs_platform_read(File *file, void *buffer, usize size, usize *bytes_read) {
    ssize_t result;

    if (file == NULL || buffer == NULL)
        return FS_ERROR;

    if (bytes_read != NULL)
        *bytes_read = 0;

    if (size == 0)
        return FS_OK;

    if (size > (usize)SSIZE_MAX)
        size = (usize)SSIZE_MAX;

    result = read(file->fd, buffer, (size_t)size);

    if (result < 0)
        return fs_posix_error(errno);

    if (bytes_read != NULL)
        *bytes_read = (usize)result;

    return FS_OK;
}

FileResult fs_platform_write(File *file, const void *buffer, usize size, usize *bytes_written) {
    ssize_t result;

    if (file == NULL || buffer == NULL)
        return FS_ERROR;

    if (bytes_written != NULL)
        *bytes_written = 0;

    if (size == 0)
        return FS_OK;

    if (size > (usize)SSIZE_MAX)
        size = (usize)SSIZE_MAX;

    result = write(file->fd, buffer, (size_t)size);

    if (result < 0)
        return fs_posix_error(errno);

    if (bytes_written != NULL)
        *bytes_written = (usize)result;

    return FS_OK;
}

FileResult fs_platform_seek(File *file, u64 position) {
    off_t result;

    if (file == NULL)
        return FS_ERROR;

    if (position > (u64)INT64_MAX)
        return FS_ERROR;

    result = lseek(file->fd, (off_t)position, SEEK_SET);

    if (result == (off_t)-1)
        return fs_posix_error(errno);

    return FS_OK;
}

FileResult fs_platform_tell(File *file, u64 *position) {
    off_t result;

    if (file == NULL || position == NULL)
        return FS_ERROR;

    result = lseek(file->fd, 0, SEEK_CUR);

    if (result == (off_t)-1)
        return fs_posix_error(errno);

    *position = (u64)result;

    return FS_OK;
}

FileResult fs_platform_stat(str8 path, FileInfo *info) {
    struct stat st;

    if (path == NULL || info == NULL)
        return FS_ERROR;

    memset(info, 0, sizeof(*info));

    /* Follow symlinks so size/type describe the target (needed by fs_read_all). */
    if (stat(path, &st) == -1)
        return fs_posix_error(errno);

    if (S_ISREG(st.st_mode)) {
        info->type = FS_FILE;
    } else if (S_ISDIR(st.st_mode)) {
        info->type = FS_DIRECTORY;
    } else {
        info->type = FS_OTHER;
    }

    if (st.st_size >= 0)
        info->size = (u64)st.st_size;

    return FS_OK;
}

FileResult fs_platform_exists(str8 path) {
    if (path == NULL)
        return FS_INVALID_PATH;

    if (access(path, F_OK) == 0)
        return FS_OK;

    return fs_posix_error(errno);
}

FileResult fs_platform_remove(str8 path) {
    struct stat st;

    if (path == NULL)
        return FS_INVALID_PATH;

    if (lstat(path, &st) == -1)
        return fs_posix_error(errno);

    if (S_ISDIR(st.st_mode)) {
        if (rmdir(path) == -1)
            return fs_posix_error(errno);
    } else {
        if (unlink(path) == -1)
            return fs_posix_error(errno);
    }

    return FS_OK;
}

FileResult fs_platform_rename(str8 old_path, str8 new_path) {
    if (old_path == NULL || new_path == NULL)
        return FS_INVALID_PATH;

    if (rename(old_path, new_path) == -1)
        return fs_posix_error(errno);

    return FS_OK;
}

FileResult fs_platform_mkdir(str8 path) {
    if (path == NULL)
        return FS_INVALID_PATH;

    if (mkdir(path, 0777) == -1)
        return fs_posix_error(errno);

    return FS_OK;
}

FileResult fs_platform_opendir(str8 path, Directory **directory) {
    Directory *result;
    DIR *dir;

    if (path == NULL || directory == NULL)
        return FS_INVALID_PATH;

    *directory = NULL;

    dir = opendir(path);

    if (dir == NULL)
        return fs_posix_error(errno);

    result = utils_malloc(sizeof(*result));

    if (result == NULL) {
        closedir(dir);
        return FS_NO_SPACE;
    }

    result->dir = dir;

    *directory = result;

    return FS_OK;
}

FileResult fs_platform_readdir(Directory *directory, DirectoryEntry *entry) {
    struct dirent *dirent;

    if (directory == NULL || entry == NULL)
        return FS_ERROR;

    errno = 0;

    dirent = readdir(directory->dir);

    if (dirent == NULL) {
        if (errno != 0)
            return fs_posix_error(errno);

        return FS_NOT_FOUND;
    }

    memset(entry, 0, sizeof(*entry));

    strncpy(entry->name, dirent->d_name, FS_MAX_NAME - 1);

    entry->name[FS_MAX_NAME - 1] = '\0';

    switch (dirent->d_type) {
    case DT_REG:
        entry->type = FS_FILE;
        break;

    case DT_DIR:
        entry->type = FS_DIRECTORY;
        break;

    case DT_LNK:
        entry->type = FS_SYMLINK;
        break;

    default:
        entry->type = FS_OTHER;
        break;
    }

    return FS_OK;
}

FileResult fs_platform_closedir(Directory *directory) {
    int result;

    if (directory == NULL)
        return FS_ERROR;

    result = closedir(directory->dir);

    utils_free(directory);

    if (result == -1)
        return fs_posix_error(errno);

    return FS_OK;
}

#endif /* !CUSTOM_FS && !_WIN32 */

#if defined(CUSTOM_FS) || defined(_WIN32)
/* Avoid -Wpedantic empty translation unit. */
typedef int fs_posix_unused;
#endif
