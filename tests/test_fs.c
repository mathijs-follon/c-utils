#include "c_utils/alloc.h"
#include "c_utils/fs.h"
#include "c_utils/str.h"
#include "test.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

static void make_temp_path(char *buf, usize size, const char *prefix) {
    snprintf(buf, (size_t)size, "/tmp/%s_%d", prefix, (int)getpid());
}

static void test_fs_write_read_roundtrip(void) {
    char path[256];
    make_temp_path(path, sizeof(path), "utils_fs_test");
    StringView path_view = {(usize)strlen(path), (str8)path};

    const char *payload = "hello fs\n";
    usize payload_len = strlen(payload);

    FileResult result = fs_write_all(path_view, payload, payload_len);
    TEST_EQ_INT(result, FS_OK);
    TEST_CHECK(fs_exists(path_view));

    FileInfo info;
    result = fs_stat(path_view, &info);
    TEST_EQ_INT(result, FS_OK);
    TEST_EQ_INT(info.type, FS_FILE);
    TEST_CHECK(info.size == (u64)payload_len);

    void *data = NULL;
    usize size = 0;
    result = fs_read_all(path_view, &data, &size);
    TEST_EQ_INT(result, FS_OK);
    TEST_EQ_USIZE(size, payload_len);
    TEST_CHECK(data != NULL);
    TEST_CHECK(memcmp(data, payload, payload_len) == 0);
    utils_free(data);

    File *file = NULL;
    result = fs_open(path_view, &file);
    TEST_EQ_INT(result, FS_OK);
    TEST_CHECK(file != NULL);

    char buf[16];
    usize n = 0;
    result = fs_read(file, buf, 5, &n);
    TEST_EQ_INT(result, FS_OK);
    TEST_EQ_USIZE(n, 5);
    TEST_CHECK(memcmp(buf, "hello", 5) == 0);

    u64 pos = 0;
    TEST_EQ_INT(fs_tell(file, &pos), FS_OK);
    TEST_CHECK(pos == 5);
    TEST_EQ_INT(fs_seek(file, 0), FS_OK);
    TEST_EQ_INT(fs_tell(file, &pos), FS_OK);
    TEST_CHECK(pos == 0);

    TEST_EQ_INT(fs_close(file), FS_OK);

    TEST_EQ_INT(fs_remove(path_view), FS_OK);
    TEST_CHECK(!fs_exists(path_view));
}

static void test_fs_mkdir_and_dir(void) {
    char dirpath[256];
    char filepath[300];
    make_temp_path(dirpath, sizeof(dirpath), "utils_fs_dir");
    snprintf(filepath, sizeof(filepath), "%s/file.txt", dirpath);

    StringView dir_view = {(usize)strlen(dirpath), (str8)dirpath};
    StringView file_view = {(usize)strlen(filepath), (str8)filepath};

    TEST_EQ_INT(fs_mkdir(dir_view), FS_OK);
    TEST_EQ_INT(fs_write_all(file_view, "x", 1), FS_OK);

    Directory *dir = NULL;
    TEST_EQ_INT(fs_opendir(dir_view, &dir), FS_OK);
    TEST_CHECK(dir != NULL);

    bool found = false;
    for (;;) {
        DirectoryEntry entry;
        FileResult r = fs_readdir(dir, &entry);
        if (r == FS_NOT_FOUND)
            break;
        TEST_EQ_INT(r, FS_OK);
        if (strcmp(entry.name, "file.txt") == 0) {
            found = true;
            TEST_EQ_INT(entry.type, FS_FILE);
        }
    }
    TEST_CHECK(found);
    TEST_EQ_INT(fs_closedir(dir), FS_OK);

    TEST_EQ_INT(fs_remove(file_view), FS_OK);
    TEST_EQ_INT(fs_remove(dir_view), FS_OK);
}

static void test_fs_strerror(void) {
    StringView msg = fs_strerror(FS_OK);
    TEST_CHECK(string_view_equal(msg, CSTR_TO_VIEW("success")));
    msg = fs_strerror(FS_NOT_FOUND);
    TEST_CHECK(string_view_contains(msg, CSTR_TO_VIEW("not found")));
}

int main(void) {
    test_fs_write_read_roundtrip();
    test_fs_mkdir_and_dir();
    test_fs_strerror();
    return test_report("fs");
}
