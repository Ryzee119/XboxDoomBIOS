// DVD ISO9660 filesystem driver

#include <lib9660/lib9660.h>

#include "main.h"

#if (1)
// // given "c:/foo/bar/really_long_folder_name/my_file_is_here.txt" to "C:/FOO/BAR/REALLY_L/MY_FILE_.TXT;1"
static void convert_path_to_iso9660(const char *input, char *output, uint8_t level, size_t output_size)
{
    memset(output, 'a', output_size);
    char *output_start = output;
    strncpy(output, input, output_size - 1);
    output[output_size - 1] = '\0';

    if (output[1] == ':') {
        output[0] = toupper((unsigned char)output[0]);
        output[1] = ':';
        output[2] = '/';
        output += 3;
    }

    char *token = strtok(output, "/\\");
    char *next_token = strtok(NULL, "/\\");

    while (token != NULL) {
        // If this is the last token, it's a file. Otherwise, it's a directory.
        if (next_token == NULL) {
            // Split off the extension first.
            char *extension = strrchr(token, '.');
            size_t extension_length = 0;

            // Replace all invalid characters with underscores. Valid characters are A-Z, 0-9, and _.
            for (size_t i = 0; i < strlen(token); i++) {
                if (!isalnum(token[i])) {
                    token[i] = '_';
                } else {
                    token[i] = toupper((unsigned char)token[i]);
                }
            }

            // If there is an extension, split it off and cap it at 3 chars for level 1 isos
            if (extension) {
                *extension = '\0';
                extension++;

                extension_length = strlen(extension);

                // level 1 isos only support 3 char extensions, so truncate if needed
                if (level == 1 && extension_length > 3) {
                    extension[3] = '\0';
                    extension_length = 3;
                }
            }

            // Cap the filename length at 8 chars for level 1 isos
            size_t file_name_length = strlen(token);
            if (level == 1 && file_name_length > 8) {
                token[8] = '\0';
                file_name_length = 8;
            }

            // Add a dot back in. Even if no extension ISO standard expect a dot.
            token[file_name_length++] = '.';

            // Put the extension after the filename
            memmove(&token[file_name_length], extension, extension_length);
            file_name_length += extension_length;

            // Finally add the version number ;1 which is required by ISO9660
            strcpy(&token[file_name_length], ";1");
            file_name_length += 2;

            output += file_name_length;
        } else {
            // Process a directory name.
            size_t token_length = strlen(token);
            size_t max_length = (level == 1) ? 8 : 31;
            if (token_length > max_length) {
                token[max_length] = '\0';
            }
            const size_t folder_name_length = strlen(token);

            // Replace all invalid characters with underscores. Valid characters are A-Z, 0-9, and _.
            // Also convert to uppercase since ISO9660 is case insensitive and most tools uppercase names
            for (size_t i = 0; i < folder_name_length; i++) {
                if (!isalnum(token[i])) {
                    token[i] = '_';
                } else {
                    token[i] = toupper((unsigned char)token[i]);
                }
            }

            // Re-add the directory separator that strtok removed
            output += folder_name_length;
            *output++ = '/';
        }

        // Advance the tokens
        token = next_token;
        if (token != NULL) {
            next_token = strtok(NULL, "/\\");
        }
    }
}

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

    const size_t path_len = strlen(path);
    char *converted_path = pvPortMalloc(path_len + 3);
    if (dir == NULL || file == NULL || converted_path == NULL) {
        if (dir) {
            vPortFree(dir);
        }
        if (file) {
            vPortFree(file);
        }
        if (converted_path) {
            vPortFree(converted_path);
        }
        return NULL;
    }
    memset(dir, 0, sizeof(l9660_dir));

    // Try 31 char filename first, then fallback to 8 char if that fails
    const char *file_start;
    convert_path_to_iso9660(path, converted_path, 2, path_len + 1);

    file_start = strrchr(converted_path, '/');
    file_start++;
    parent = open_dir_recurse(fs, dir, converted_path);
    status = (parent) ? l9660_openat(file, parent, file_start) : L9660_ENOENT;

    if (status != L9660_OK) {
        convert_path_to_iso9660(path, converted_path, 1, path_len + 1);
        file_start = strrchr(converted_path, '/');
        file_start++;
        parent = open_dir_recurse(fs, dir, converted_path);
        status = (parent) ? l9660_openat(file, parent, file_start) : L9660_ENOENT;
        if (status != L9660_OK) {
            vPortFree(file);
            vPortFree(dir);
            vPortFree(converted_path);
            return NULL;
        }
    }

    status = l9660_openat(file, parent, file_start);
    vPortFree(dir);
    vPortFree(converted_path);
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
    const size_t path_len = strlen(path);
    char *converted_path = pvPortMalloc(path_len + 3);
    if (dir == NULL || converted_path == NULL) {
        if (dir) {
            vPortFree(dir);
        }
        if (converted_path) {
            vPortFree(converted_path);
        }
        return NULL;
    }

    l9660_fs *fs = &fs_user->fs;


    convert_path_to_iso9660(path, converted_path, 2, path_len + 1);
    parent = open_dir_recurse(fs, dir, converted_path);
    if (parent == NULL) {
        convert_path_to_iso9660(path, converted_path, 1, path_len + 1);
        parent = open_dir_recurse(fs, dir, converted_path);
        if (parent == NULL) {
            vPortFree(dir);
            vPortFree(converted_path);
            return NULL;
        }
    }

    return (user_dir_handle_t *)dir;
}

struct directory_entry *iso9660_readdir(user_dir_handle_t *handle, struct directory_entry *entry)
{
    l9660_dir *dir = (l9660_dir *)handle;
    char temp[300];
    l9660_dirent *dent = (l9660_dirent *)temp;
    l9660_status status = l9660_readdir(dir, &dent);

    if (status != L9660_OK || dent == NULL) {
        return NULL;
    }
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    memcpy(&entry->file_size, dent->size.le, sizeof(entry->file_size));
#else
    memcpy(&entry->file_size, dent->size.be, sizeof(entry->file_size));
#endif
    const size_t name_len =
        (dent->name_len < sizeof(entry->file_name) - 1) ? dent->name_len : sizeof(entry->file_name) - 1;
    strncpy(entry->file_name, dent->name, name_len);
    entry->file_name[name_len] = '\0';
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
    char path_len = strlen(path_copy);

    l9660_dir temp;

    while ((seg_end = strchr(seg_start, '/')) != NULL) {
        *seg_end = '\0';
        uint32_t len = seg_end - seg_start;
        assert(len > 0);

        if (len >= 2 && seg_start[1] == ':') {
            status = l9660_fs_open_root(&temp, fs);
        } else {
            status = l9660_opendirat(&temp, dir, seg_start);
        }
        if (status != L9660_OK) {
            vPortFree(path_copy);
            return NULL;
        }
        memcpy(dir, &temp, sizeof(l9660_dir));
        seg_start = seg_end + 1;
    }
    vPortFree(path_copy);
    return dir;
}
