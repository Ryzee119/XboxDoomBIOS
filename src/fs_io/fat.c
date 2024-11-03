// FAT/EXFAT

#include <fatfs/ff.h>
#include <fatfs/diskio.h>

#include "main.h"

user_fs_handle_t *fat_init(file_io_driver_t *driver, void *arg)
{
    (void)arg;
    
    char drive_letter = driver->drive_letter;

    user_fs_handle_t *handle = pvPortMalloc(sizeof(FATFS));
    if (handle == NULL) {
        return NULL;
    }
    FATFS *fs = (FATFS *)handle;

    char path[3] = {drive_letter, ':', '\0'};
    if (f_mount(fs, path, 1) != FR_OK) {
        vPortFree(handle);
        printf_ts("[FATFS] Failed to mount drive %c\n", drive_letter);
        return NULL;
    }

    return handle;
}

void fat_deinit(user_fs_handle_t *handle)
{
    FATFS *fs = (FATFS *)handle;
    char path[3] = {'0' + fs->pdrv, ':', '\0'};
    f_unmount(path);
    vPortFree(handle);
}

user_file_handle_t *fat_open(user_fs_handle_t *handle, const char *path, int flags)
{
    (void) handle;
    BYTE mode = 0;
    if ((flags & O_RDONLY) == O_RDONLY) {
        mode = FA_READ;
    } else if ((flags & O_WRONLY) == O_WRONLY) {
        mode = FA_WRITE;
    } else if ((flags & O_RDWR) == O_RDWR) {
        mode = FA_READ | FA_WRITE;
    }
    if (flags & O_CREAT) {
        mode |= FA_CREATE_ALWAYS;
    }
    FIL *fp = pvPortMalloc(sizeof(FIL));
    FRESULT result = f_open(fp, path, mode);
    return (result == FR_OK) ? (user_file_handle_t *)fp : NULL;
}

ssize_t fat_read(user_file_handle_t *fd, void *buffer, size_t count)
{
    FIL *fp = (FIL *)fd;
    UINT bytes_read;
    FRESULT result = f_read(fp, buffer, count, &bytes_read);
    return (result == FR_OK) ? bytes_read : -1;
}

ssize_t fat_write(user_file_handle_t *fd, const void *buffer, size_t count)
{
    FIL *fp = (FIL *)fd;
    UINT bytes_written;
    FRESULT result = f_write(fp, buffer, count, &bytes_written);
    return (result == FR_OK) ? bytes_written : -1;
}

off_t fat_lseek(user_file_handle_t *fd, off_t offset, int whence)
{
    FIL *fp = (FIL *)fd;
    FSIZE_t file_end = f_size(fp);
    FSIZE_t file_current = f_tell(fp);

    DWORD adjusted_offset = 0;
    if (whence == SEEK_SET) {
        adjusted_offset = offset;
    } else if (whence == SEEK_CUR) {
        adjusted_offset = file_current + offset;
    } else if (whence == SEEK_END) {
        adjusted_offset = file_end + offset;
    }

    FRESULT result = f_lseek(fp, adjusted_offset);
    return (result == FR_OK) ? adjusted_offset : -1;
}

int fat_close(user_file_handle_t *fd)
{
    FIL *fp = (FIL *)fd;
    f_close(fp);
    vPortFree(fp);
    return 0;
}

user_dir_handle_t *fat_opendir(user_fs_handle_t *handle, const char *path)
{
    (void) handle;
    DIR *dir = pvPortMalloc(sizeof(DIR));
    if (dir == NULL) {
        return NULL;
    }

    FRESULT result = f_opendir(dir, path);
    if (result != FR_OK) {
        vPortFree(dir);
        return NULL;
    }

    return (result == FR_OK) ? (user_dir_handle_t *)dir : NULL;
}

struct directory_entry *fat_readdir(user_dir_handle_t *handle, struct directory_entry *entry)
{
    FILINFO fno;
    FRESULT result = f_readdir((DIR *)handle, &fno);
    if (result != FR_OK || fno.fname[0] == 0) {
        return NULL;
    }

    entry->file_size = fno.fsize;
    strncpy(entry->file_name, fno.fname, sizeof(entry->file_name));
    return entry;
}

void fat_closedir(user_dir_handle_t *handle)
{
    f_closedir((DIR *)handle);
    vPortFree(handle);
}

fs_io_t fat_io = {
    .init = fat_init,
    .deinit = fat_deinit,
    .open = fat_open,
    .read = fat_read,
    .write = fat_write,
    .lseek = fat_lseek,
    .close = fat_close,
    .opendir = fat_opendir,
    .readdir = fat_readdir,
    .closedir = fat_closedir,
};

// Callbacks from internal FATFS driver
DSTATUS disk_status(BYTE pdrv)
{
    char drive_letter = '0' + pdrv;
    file_io_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return STA_NOINIT;
    }

    return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    char drive_letter = '0' + pdrv;
    file_io_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return RES_ERROR;
    }

    ssize_t status = driver->io_ll->read(driver->ll_handle, buff, sector, count);
    if (status < 0) {
        printf_ts("[FATFS] disk_read failed with status %d\n", status);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    char drive_letter = '0' + pdrv;
    file_io_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return RES_ERROR;
    }

    ssize_t status = driver->io_ll->write(driver->ll_handle, buff, sector, count);
    if (status < 0) {
        printf_ts("[FATFS] disk_read failed with status %d\n", status);
        return RES_ERROR;
    }
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    char drive_letter = '0' + pdrv;
    file_io_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return RES_ERROR;
    }

    switch (cmd) {
        case CTRL_SYNC:
            cmd = FS_IO_SYNC;
            break;
        case GET_SECTOR_COUNT:
            cmd = FS_IO_GET_SECTOR_COUNT;
            break;
        case GET_SECTOR_SIZE:
            cmd = FS_IO_GET_SECTOR_SIZE;
            break;
        case GET_BLOCK_SIZE:
            cmd = FS_IO_GET_BLOCK_SIZE;
            break;
        default: {
            printf_ts("[FATFS] disk_ioctl unsupported command %d\n", cmd);
            return RES_ERROR;
        }
    }

    int8_t status = driver->io_ll->ioctrl(driver->ll_handle, cmd, buff);
    if (status != 0) {
        printf_ts("[FATFS] disk_ioctl failed with status %d\n", status);
    }
    return (status == 0) ? RES_OK : RES_ERROR;
}
