#ifndef FS_H
#define FS_H

#include "c_utils/str.h"
#include "c_utils/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Portable filesystem API.
 *
 * Platform backends: fs_posix.c / fs_windows.c by default.
 * Override with CUSTOM_FS and implement the hooks in fs_platform.h:
 *
 *     cmake -DCUSTOM_FS=ON ...
 *     // then provide fs_platform_open, fs_platform_read, ...
 */

typedef enum {
    FS_OK = 0,
    FS_ERROR,
    FS_NOT_FOUND,
    FS_PERMISSION,
    FS_EXISTS,
    FS_INVALID_PATH,
    FS_NOT_DIRECTORY,
    FS_IS_DIRECTORY,
    FS_NO_SPACE,
    FS_IO_ERROR,
} FileResult;

typedef enum {
    FS_FILE,
    FS_DIRECTORY,
    FS_SYMLINK,
    FS_OTHER,
} FileType;

typedef struct {
    FileType type;
    u64 size;
} FileInfo;

typedef struct File File;
typedef struct Directory Directory;

#define FS_MAX_NAME 256

typedef struct {
    char name[FS_MAX_NAME];
    FileType type;
} DirectoryEntry;

FileResult fs_open(StringView path, File **file);

FileResult fs_create(StringView path, File **file);

FileResult fs_close(File *file);

FileResult fs_read(File *file, void *buffer, usize size, usize *bytes_read);

FileResult fs_write(File *file, const void *buffer, usize size, usize *bytes_written);

FileResult fs_seek(File *file, u64 position);

FileResult fs_tell(File *file, u64 *position);

FileResult fs_read_all(StringView path, void **data, usize *size); /* *data: utils_free */

FileResult fs_write_all(StringView path, const void *data, usize size);

FileResult fs_remove(StringView path);

FileResult fs_rename(StringView old_path, StringView new_path);

FileResult fs_stat(StringView path, FileInfo *info);

bool fs_exists(StringView path);

FileResult fs_mkdir(StringView path);

FileResult fs_opendir(StringView path, Directory **directory);

FileResult fs_readdir(Directory *directory, DirectoryEntry *entry);

FileResult fs_closedir(Directory *directory);

StringView fs_strerror(FileResult result);

#ifdef __cplusplus
}
#endif

#endif /* FS_H */
