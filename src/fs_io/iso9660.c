// DVD ISO9660 filesystem driver

#include <lib9660/lib9660.h>

#include "main.h"

#if (1)
static l9660_dir *open_dir_recurse(l9660_fs *fs, l9660_dir *dir, const char *path);

typedef struct l9660_user_fs
{
    l9660_fs fs;
    file_io_driver_t *driver;
} l9660_user_fs_t;

static bool read_sector(l9660_fs *fs, void *buf, uint32_t sector)
{
    l9660_user_fs_t *fs_user = (l9660_user_fs_t *)fs;
    file_io_driver_t *driver = fs_user->driver;
    int8_t status = driver->io_ll->read(driver->ll_handle, buf, sector, 1);
    return (status == 0);
}

user_fs_handle_t *iso9660_init(file_io_driver_t *driver, void *arg)
{
    (void)arg;
    l9660_status status;

    user_fs_handle_t *handle = pvPortMalloc(sizeof(l9660_user_fs_t));
    if (handle == NULL) {
        return NULL;
    }
    l9660_user_fs_t *fs_user = (l9660_user_fs_t *)handle;
    fs_user->driver = driver;
    status = l9660_openfs(&fs_user->fs, read_sector);
    if (status != L9660_OK) {
        vPortFree(fs_user);
        return NULL;
    }

    return handle;
}

void iso9660_deinit(user_fs_handle_t *handle)
{
    vPortFree(handle);
}

user_file_handle_t *iso9660_open(user_fs_handle_t *handle, const char *path, int flags)
{
    l9660_status status;
    l9660_user_fs_t *fs_user = (l9660_user_fs_t *)handle;
    l9660_fs *fs = &fs_user->fs;
    l9660_dir *parent;

    l9660_dir *dir = pvPortMalloc(sizeof(l9660_dir));
    l9660_file *file = pvPortMalloc(sizeof(l9660_file));
    if (dir == NULL || file == NULL) {
        if (dir) {
            vPortFree(dir);
        }
        if (file) {
            vPortFree(file);
        }
        return NULL;
    }
    memset(dir, 0, sizeof(l9660_dir));

    const char *file_start = strrchr(path, '/');
    parent = open_dir_recurse(fs, dir, path);

    if (parent == NULL || file_start == NULL) {
        vPortFree(file);
        vPortFree(dir);
        return NULL;
    }

    file_start++;

    status = l9660_openat(file, parent, file_start);
    vPortFree(dir);
    if (status != L9660_OK) {
        vPortFree(file);
        return NULL;
    }

    return (user_file_handle_t *)file;
}

ssize_t iso9660_read(user_file_handle_t *fd, void *buffer, size_t count)
{
    l9660_file *file = (l9660_file *)fd;
    size_t bytes_read;
    l9660_status status = l9660_read(file, buffer, count, &bytes_read);
    return (status == L9660_OK) ? bytes_read : -1;
}

ssize_t iso9660_write(user_file_handle_t *fd, const void *buffer, size_t count)
{
    return -1;
}

off_t iso9660_lseek(user_file_handle_t *fd, off_t offset, int whence)
{
    l9660_file *file = (l9660_file *)fd;
    l9660_status status = l9660_seek(file, whence, offset);
    return (status == L9660_OK) ? l9660_tell(file) : -1;
}

int iso9660_close(user_file_handle_t *fd)
{
    l9660_file *file = (l9660_file *)fd;
    vPortFree(file);
    return 0;
}

user_dir_handle_t *iso9660_opendir(user_fs_handle_t *handle, const char *path)
{
    l9660_user_fs_t *fs_user = (l9660_user_fs_t *)handle;
    l9660_dir *parent;
    l9660_dir *dir = pvPortMalloc(sizeof(l9660_dir));
    if (dir == NULL) {
        return NULL;
    }

    l9660_fs *fs = &fs_user->fs;

    parent = open_dir_recurse(fs, dir, path);
    if (parent == NULL) {
        vPortFree(dir);
        return NULL;
    }

    return (user_dir_handle_t *)dir;
}

struct directory_entry *iso9660_readdir(user_dir_handle_t *handle, struct directory_entry *entry)
{
    l9660_dir *dir = (l9660_dir *)handle;
    l9660_dirent *dent;
    l9660_status status = l9660_readdir(dir, &dent);

    if (status != L9660_OK || dent == NULL) {
        return NULL;
    }
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    memcpy(&entry->file_size, dent->size.le, sizeof(entry->file_size));
#else
    memcpy(&entry->file_size, dent->size.be, sizeof(entry->file_size));
#endif
    strncpy(entry->file_name, dent->name, sizeof(entry->file_name));
    return entry;
}

void iso9660_closedir(user_dir_handle_t *handle)
{
    l9660_dir *dir = (l9660_dir *)handle;
    vPortFree(dir);
}

fs_io_t iso9660_io = {
    .init = iso9660_init,
    .deinit = iso9660_deinit,
    .open = iso9660_open,
    .read = iso9660_read,
    .write = iso9660_write,
    .lseek = iso9660_lseek,
    .close = iso9660_close,
    .opendir = iso9660_opendir,
    .readdir = iso9660_readdir,
    .closedir = iso9660_closedir,
};

#endif

static l9660_dir *open_dir_recurse(l9660_fs *fs, l9660_dir *dir, const char *path)
{
    // given "c:/foo/bar/file.txt", open each directory in turn then return the handle to the last directory (bar/)
    l9660_status status;

    char *path_copy = pvPortMalloc(strlen(path) + 1);
    if (path_copy == NULL) {
        return NULL;
    }
    strcpy(path_copy, path);

    char *seg_start = path_copy;
    char *seg_end = NULL;

    while ((seg_end = strchr(seg_start, '/')) != NULL) {
        *seg_end = '\0';
        uint32_t len = seg_end - seg_start;
        assert(len > 0);

        if (len >= 2 && seg_start[1] == ':') {
            status = l9660_fs_open_root(dir, fs);
        } else {
            status = l9660_opendirat(dir, dir, seg_start);
        }
        if (status != L9660_OK) {
            vPortFree(path_copy);
            return NULL;
        }
        seg_start = seg_end + 1;
    }
    vPortFree(path_copy);
    return dir;
}