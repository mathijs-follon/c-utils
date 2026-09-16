#ifndef FS_PLATFORM_H
#define FS_PLATFORM_H

#include "c_utils/fs.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Platform backend for fs.h.
 *
 * Default backends live in fs_posix.c / fs_windows.c.
 *
 * To supply your own:
 *
 *   1. Build utils with -DCUSTOM_FS (CMake: -DCUSTOM_FS=ON)
 *   2. Define opaque File / Directory storage in your .c
 *   3. Implement every fs_platform_* below and link it
 *
 *     #define CUSTOM_FS
 *     // or: cmake -DCUSTOM_FS=ON
 *
 *     #include "c_utils/fs_platform.h"
 *     #include "c_utils/alloc.h"
 *
 *     struct File { ... };
 *     struct Directory { ... };
 *
 *     FileResult fs_platform_open(str8 path, File **file) { ... }
 *     // ...
 *
 * Paths are NUL-terminated UTF-8 C strings (str8).
 * Allocate File/Directory with utils_malloc; free with utils_free.
 * fs_platform_exists should return FS_OK when the path exists.
 */

FileResult fs_platform_open(str8 path, File **file);
FileResult fs_platform_create(str8 path, File **file);
FileResult fs_platform_close(File *file);

FileResult fs_platform_read(File *file, void *buffer, usize size, usize *bytes_read);
FileResult fs_platform_write(File *file, const void *buffer, usize size, usize *bytes_written);

FileResult fs_platform_seek(File *file, u64 position);
FileResult fs_platform_tell(File *file, u64 *position);

FileResult fs_platform_stat(str8 path, FileInfo *info);
FileResult fs_platform_exists(str8 path);
FileResult fs_platform_remove(str8 path);
FileResult fs_platform_rename(str8 old_path, str8 new_path);
FileResult fs_platform_mkdir(str8 path);

FileResult fs_platform_opendir(str8 path, Directory **directory);
FileResult fs_platform_readdir(Directory *directory, DirectoryEntry *entry);
FileResult fs_platform_closedir(Directory *directory);

#ifdef __cplusplus
}
#endif

#endif /* FS_PLATFORM_H */
