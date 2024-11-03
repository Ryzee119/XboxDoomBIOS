// FATX

#include <libfatx/fatx.h>
#include <libfatx/fatx_internal.h>

#include "main.h"

#define FATX_MIN(a, b) ((a) < (b) ? (a) : (b))

typedef struct fatx_fs_file
{
    struct fatx_fs *fs;
    uint32_t cursor;
    struct fatx_attr attr;
    int flags;
    char path[255];
    uint8_t sector_cache[ATA_SECTOR_SIZE];
} fatx_file_t;

typedef struct fatx_fs_dir
{
    struct fatx_fs *fs;
    struct fatx_dir dir;
    struct fatx_dirent entry;
    char path[255];
} fatx_dir_t;

typedef struct fatx_extra_data
{
    file_io_driver_t *driver;
    uint64_t seek_offset;
    uint64_t cached_sector;
    uint16_t sector_size;
    uint8_t sector_cache[] __attribute__((aligned(4)));
} fatx_extra_data_t;

user_fs_handle_t *fatx_fs_init(file_io_driver_t *driver, void *arg)
{
    (void)arg;

    char drive_letter = driver->drive_letter;

    user_fs_handle_t *handle = pvPortMalloc(sizeof(struct fatx_fs));
    if (handle == NULL) {
        return NULL;
    }

    uint16_t sector_size;
    driver->io_ll->ioctrl(driver->ll_handle, FS_IO_GET_SECTOR_SIZE, &sector_size);

    struct fatx_fs *fatx = (struct fatx_fs *)(handle);
    fatx->user_data = pvPortMalloc(sizeof(fatx_extra_data_t) + 2048);
    if (fatx->user_data == NULL) {
        vPortFree(fatx);
        return NULL;
    }

    fatx_extra_data_t *fatx_extra_data = (fatx_extra_data_t *)fatx->user_data;
    fatx_extra_data->sector_size = sector_size;
    fatx_extra_data->seek_offset = 0;
    fatx_extra_data->driver = driver;
    fatx_extra_data->cached_sector = 0xFFFFFFFFFFFFFFFF;

    char path[4] = {'/', drive_letter, '\0'};

    uint64_t partition_offset, partition_size;
    if (fatx_drive_to_offset_size(tolower(drive_letter), &partition_offset, &partition_size) != FATX_STATUS_SUCCESS) {
        printf_ts("[FATX] Failed to get partition offset and size\n");
        vPortFree(fatx->user_data);
        vPortFree(fatx);
        return NULL;
    }

    if (fatx_open_device(fatx, path, partition_offset, partition_size, sector_size, FATX_READ_FROM_SUPERBLOCK) !=
        FATX_STATUS_SUCCESS) {
        printf_ts("[FATX] Failed to mount drive %c\n", drive_letter);
        vPortFree(fatx->user_data);
        vPortFree(fatx);
        return NULL;
    }

    return handle;
}

void fatx_fs_deinit(user_fs_handle_t *user_data)
{
    struct fatx_fs *fatx = (struct fatx_fs *)(user_data);
    fatx_close_device(fatx);
    vPortFree(fatx->user_data);
    vPortFree(fatx);
}

user_file_handle_t *fatx_fs_open(user_fs_handle_t *handle, const char *path, int flags)
{
    struct fatx_fs *fs = (struct fatx_fs *)handle;

    fatx_file_t *file = pvPortMalloc(sizeof(fatx_file_t));
    if (file == NULL) {
        return NULL;
    }

    file->fs = fs;
    file->cursor = 0;
    file->flags = flags;

    // Drop the drive letter and colon from the path
    strncpy(file->path, path + 2, sizeof(file->path));

    if (flags & O_CREAT) {
        uint8_t file_exists = fatx_get_attr(fs, file->path, &file->attr) == FATX_STATUS_SUCCESS;

        if (file_exists == 0) {
            if (fatx_mknod(fs, file->path) != FATX_STATUS_SUCCESS) {
                vPortFree(file);
                return NULL;
            }
        }
        if (flags & O_EXCL && file_exists) {
            printf_ts("[FATX] File already exists\n");
            vPortFree(file);
            return NULL;
        }
        if (flags & O_TRUNC) {
            if (fatx_truncate(fs, file->path, 0) != FATX_STATUS_SUCCESS) {
                printf_ts("[FATX] Failed to truncate file\n");
                vPortFree(file);
                return NULL;
            }
        }
    }

    if (fatx_get_attr(fs, file->path, &file->attr) != FATX_STATUS_SUCCESS) {
        vPortFree(file);
        return NULL;
    }

    // Readonly file opened for Write
    if ((flags & O_RDWR || flags & O_WRONLY) && (file->attr.attributes & FATX_ATTR_READ_ONLY)) {
        vPortFree(file);
        return NULL;
    }

    // On append, cursor moved to end of the file
    if (flags & O_APPEND) {
        file->cursor = file->attr.file_size;
    }

    return (user_file_handle_t *)file;
}

ssize_t fatx_fs_read(user_file_handle_t *fd, void *buffer, size_t count)
{
    fatx_file_t *file = (fatx_file_t *)fd;
    struct fatx_fs *fs = file->fs;

    if (file->flags & O_APPEND) {
        return -1;
    }

    ssize_t bytes_transferred = fatx_read(fs, file->path, file->cursor, count, buffer);
    if (bytes_transferred > 0) {
        file->cursor += bytes_transferred;
    }
    return bytes_transferred;
}

ssize_t fatx_fs_write(user_file_handle_t *fd, const void *buffer, size_t count)
{
    fatx_file_t *file = (fatx_file_t *)fd;
    struct fatx_fs *fs = file->fs;

    if (file->flags & O_APPEND) {
        return -1;
    }

    ssize_t bytes_transferred = fatx_write(fs, file->path, file->cursor, count, buffer);
    if (bytes_transferred > 0) {
        file->cursor += bytes_transferred;
    }
    return bytes_transferred;
}

off_t fatx_fs_lseek(user_file_handle_t *fd, off_t offset, int whence)
{

    fatx_file_t *file = (fatx_file_t *)fd;
    struct fatx_fs *fs = file->fs;
    uint32_t file_end = file->attr.file_size;

    uint32_t adjusted_offset = 0;
    if (whence == SEEK_SET) {
        adjusted_offset = offset;
    } else if (whence == SEEK_CUR) {
        adjusted_offset = file->cursor + offset;
    } else if (whence == SEEK_END) {
        adjusted_offset = file_end + offset;
    }

    // Pad out the file with zeroes if seek past end
    if (adjusted_offset > file_end && (file->flags & O_WRONLY || file->flags & O_RDWR)) {
        uint32_t bytes_to_write = adjusted_offset - file_end;
        uint32_t chunk = FATX_MIN(bytes_to_write, 512);
        uint8_t *zeroes = pvPortMalloc(chunk);
        if (zeroes == NULL) {
            return -1;
        }
        memset(zeroes, 0, chunk);
        file->cursor = file_end;
        while (bytes_to_write) {
            uint32_t bytes_written = fatx_write(fs, file->path, file_end, chunk, zeroes);
            if (bytes_written == 0) {
                vPortFree(zeroes);
                return -1;
            }
            file_end += bytes_written;
            bytes_to_write -= bytes_written;
        }
        vPortFree(zeroes);

        // Re-get attributes with new file size
        fatx_get_attr(file->fs, file->path, &file->attr);
    }

    file->cursor = adjusted_offset;
    return adjusted_offset;
}

int fatx_fs_close(user_file_handle_t *fd)
{
    fatx_file_t *file = (fatx_file_t *)fd;

    // Xbox can turn off any time, be aggressive with cache flushes for writes
    if (file->flags & O_WRONLY || file->flags & O_RDWR) {
        fatx_flush_fat_cache(file->fs);
    }

    vPortFree(file);
    return 0;
}

user_dir_handle_t *fatx_fs_opendir(user_fs_handle_t *handle, const char *path)
{
    struct fatx_fs *fs = (struct fatx_fs *)handle;

    fatx_dir_t *fatx_dir = pvPortMalloc(sizeof(fatx_dir_t));
    if (fatx_dir == NULL) {
        return NULL;
    }

    // Drop the drive letter and colon from the path
    strncpy(fatx_dir->path, path + 2, sizeof(fatx_dir->path));
    fatx_dir->fs = fs;

    int result = fatx_open_dir(fs, fatx_dir->path, &fatx_dir->dir);
    if (result != FATX_STATUS_SUCCESS) {
        vPortFree(fatx_dir);
        return NULL;
    }
    return (user_dir_handle_t *)fatx_dir;
}

struct directory_entry *fatx_fs_readdir(user_dir_handle_t *handle, struct directory_entry *entry)
{
    fatx_dir_t *fatx_dir = (fatx_dir_t *)handle;
    struct fatx_dirent *result;
    struct fatx_attr attr;
    int status;

    do {
        memset(&attr, 0, sizeof(attr));
        status = fatx_read_dir(fatx_dir->fs, &fatx_dir->dir, &fatx_dir->entry, &attr, &result);
        fatx_next_dir_entry(fatx_dir->fs, &fatx_dir->dir);
    } while (status == FATX_STATUS_FILE_DELETED);

    if (status != FATX_STATUS_SUCCESS || result == NULL) {
        return NULL;
    }

    strncpy(entry->file_name, result->filename, sizeof(entry->file_name));
    entry->file_size = attr.file_size;
    return entry;
}

void fatx_fs_closedir(user_dir_handle_t *handle)
{
    fatx_dir_t *fatx_dir = (fatx_dir_t *)handle;
    fatx_close_dir(fatx_dir->fs, &fatx_dir->dir);
    vPortFree(fatx_dir);
}

fs_io_t fatx_io = {
    .init = fatx_fs_init,
    .deinit = fatx_fs_deinit,
    .open = fatx_fs_open,
    .read = fatx_fs_read,
    .write = fatx_fs_write,
    .lseek = fatx_fs_lseek,
    .close = fatx_fs_close,
    .opendir = fatx_fs_opendir,
    .readdir = fatx_fs_readdir,
    .closedir = fatx_fs_closedir,
};

int fatx_dev_seek(struct fatx_fs *fs, uint64_t offset)
{
    fatx_extra_data_t *fatx_extra_data = (fatx_extra_data_t *)fs->user_data;
    fatx_extra_data->seek_offset = offset;
    return 0;
}

int fatx_dev_seek_cluster(struct fatx_fs *fs, size_t cluster, off_t offset)
{
    int status;
    uint64_t pos;

    status = fatx_cluster_number_to_byte_offset(fs, cluster, &pos);
    if (status) {
        return status;
    }

    pos += offset;

    return fatx_dev_seek(fs, pos);
}

size_t fatx_dev_read(struct fatx_fs *fs, void *buf, size_t size, size_t items)
{
    fatx_extra_data_t *fatx_extra_data = (fatx_extra_data_t *)fs->user_data;
    uint64_t seek_offset = fatx_extra_data->seek_offset;
    const uint32_t sector_size = fs->sector_size;
    const uint32_t lba_offset = seek_offset % sector_size;
    uint64_t current_lba = seek_offset / sector_size;
    uint64_t bytes_remaining = size * items;
    uint8_t *buf8 = (uint8_t *)buf;

    // Unaligned or partial read for first sector
    if (lba_offset != 0) {
        if (fatx_extra_data->cached_sector != current_lba) {
            fatx_extra_data->driver->io_ll->read(fatx_extra_data->driver->ll_handle, fatx_extra_data->sector_cache,
                                                 current_lba, 1);
            fatx_extra_data->cached_sector = current_lba;
        }

        uint32_t chunk = FATX_MIN(bytes_remaining, sector_size - lba_offset);
        memcpy(buf8, fatx_extra_data->sector_cache + lba_offset, chunk);

        bytes_remaining -= chunk;
        buf8 += chunk;
        current_lba++;
    }

    // Sector aligned read
    if (bytes_remaining >= sector_size) {
        uint32_t full_sectors = bytes_remaining / sector_size;
        uint64_t full_bytes = full_sectors * sector_size;

        fatx_extra_data->driver->io_ll->read(fatx_extra_data->driver->ll_handle, buf8, current_lba, full_sectors);

        bytes_remaining -= full_bytes;
        buf8 += full_bytes;
        current_lba += full_sectors;
    }

    // Partial read for last sector
    if (bytes_remaining > 0) {
        assert(bytes_remaining < sector_size);
        if (fatx_extra_data->cached_sector != current_lba) {
            fatx_extra_data->driver->io_ll->read(fatx_extra_data->driver->ll_handle, fatx_extra_data->sector_cache,
                                                 current_lba, 1);
            fatx_extra_data->cached_sector = current_lba;
        }
        memcpy(buf8, fatx_extra_data->sector_cache, bytes_remaining);
        bytes_remaining = 0;
    }

    assert(bytes_remaining == 0);

    fatx_extra_data->seek_offset = size * items;
    return items;
}

size_t fatx_dev_write(struct fatx_fs *fs, const void *buf, size_t size, size_t items)
{
    fatx_extra_data_t *fatx_extra_data = (fatx_extra_data_t *)fs->user_data;
    uint64_t seek_offset = fatx_extra_data->seek_offset;
    const uint32_t sector_size = fs->sector_size;
    const uint32_t lba_offset = seek_offset % sector_size;
    uint64_t current_lba = seek_offset / sector_size;
    uint64_t bytes_remaining = size * items;

    uint8_t *buf8 = (uint8_t *)buf;

    // Unaligned write for first sector
    if (lba_offset != 0) {
        if (fatx_extra_data->cached_sector != current_lba) {
            fatx_extra_data->driver->io_ll->read(fatx_extra_data->driver->ll_handle, fatx_extra_data->sector_cache,
                                                 current_lba, 1);
            fatx_extra_data->cached_sector = current_lba;
        }

        uint32_t chunk = FATX_MIN(bytes_remaining, sector_size - lba_offset);
        memcpy(fatx_extra_data->sector_cache + lba_offset, buf8, chunk);

        fatx_extra_data->driver->io_ll->write(fatx_extra_data->driver->ll_handle, fatx_extra_data->sector_cache,
                                              current_lba, 1);

        bytes_remaining -= chunk;
        buf8 += chunk;
        current_lba++;
    }

    // Sector aligned write
    if (bytes_remaining >= sector_size) {
        uint32_t full_sectors = bytes_remaining / sector_size;
        uint64_t full_bytes = full_sectors * sector_size;

        fatx_extra_data->driver->io_ll->write(fatx_extra_data->driver->ll_handle, buf8, current_lba, full_sectors);

        bytes_remaining -= full_bytes;
        buf8 += full_bytes;
        current_lba += full_sectors;
    }

    // Partial write for last sector
    if (bytes_remaining > 0) {
        assert(bytes_remaining < sector_size);
        if (fatx_extra_data->cached_sector != current_lba) {
            fatx_extra_data->driver->io_ll->read(fatx_extra_data->driver->ll_handle, fatx_extra_data->sector_cache,
                                                 current_lba, 1);
            fatx_extra_data->cached_sector = current_lba;
        }
        memcpy(fatx_extra_data->sector_cache, buf8, bytes_remaining);

        fatx_extra_data->driver->io_ll->write(fatx_extra_data->driver->ll_handle, fatx_extra_data->sector_cache,
                                              current_lba, 1);
        bytes_remaining = 0;
    }

    seek_offset += size * items;
    fatx_extra_data->seek_offset = seek_offset;
    return items;
}
