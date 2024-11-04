#include "main.h"
#include <fcntl.h>
#include <unistd.h>

int open(const char *path, int flags, ...)
{
    assert(path[1] == ':');
    char drive_letter = path[0];

    file_io_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return -1;
    }

    file_handle_t *fp = pvPortMalloc(sizeof(file_handle_t));
    if (fp == NULL) {
        return -1;
    }

    fp->driver = driver;

    xSemaphoreTake(fp->driver->mutex, portMAX_DELAY);
    fp->user_handle = fp->driver->io->open(driver->handle, path, flags);
    xSemaphoreGive(fp->driver->mutex);
    if (fp->user_handle == NULL) {
        vPortFree(fp);
        return -1;
    }

    return (int)fp;
}

int close(int fd)
{
    file_handle_t *fp = (file_handle_t *)fd;
    xSemaphoreTake(fp->driver->mutex, portMAX_DELAY);
    fp->driver->io->close(fp->user_handle);
    xSemaphoreGive(fp->driver->mutex);
    vPortFree(fp);
    return 0;
}

ssize_t read(int fd, void *buffer, size_t count)
{
    file_handle_t *fp = (file_handle_t *)fd;
    // printf_ts("Reading %d bytes from %08x -> ", count, fp);
    memset(buffer, 0, count);
    xSemaphoreTake(fp->driver->mutex, portMAX_DELAY);
    ssize_t sz = fp->driver->io->read(fp->user_handle, buffer, count);
    xSemaphoreGive(fp->driver->mutex);
#if (0)
    uint64_t checksum = 0;
    for (int i = 0; i < sz; i++) {
        if (sz == 20224 && i > (sz - 64)) {

            printf_ts("%02x ", ((uint8_t *)buffer)[i]);
            assert(((uint8_t *)buffer)[0] == 0x97);
        }
        checksum += ((uint8_t *)buffer)[i];
    }
    printf_ts("Checksum: %08x\n", checksum);
#endif
    return sz;
}

ssize_t write(int fd, const void *buffer, size_t count)
{
    file_handle_t *fp = (file_handle_t *)fd;
    return fp->driver->io->write(fp->user_handle, buffer, count);
}

off_t lseek(int fd, off_t offset, int whence)
{
    file_handle_t *fp = (file_handle_t *)fd;
    xSemaphoreTake(fp->driver->mutex, portMAX_DELAY);
    off_t off = fp->driver->io->lseek(fp->user_handle, offset, whence);
    xSemaphoreGive(fp->driver->mutex);
    return off;
}

// No dirent in picolibc, so we need to define our own dirent-like functions
directory_handle_t *opendir(const char *path)
{
    assert(path[1] == ':');
    char drive_letter = path[0];
    file_io_driver_t *driver = fileio_find_driver(drive_letter);
    if (driver == NULL) {
        return NULL;
    }

    directory_handle_t *dir = pvPortMalloc(sizeof(directory_handle_t));
    if (dir == NULL) {
        return NULL;
    }
    dir->entry.file_name[0] = 0;
    dir->entry.file_size = 0;
    dir->driver = driver;
    xSemaphoreTake(driver->mutex, portMAX_DELAY);
    dir->user_handle = dir->driver->io->opendir(dir->driver->handle, path);
    xSemaphoreGive(driver->mutex);
    if (dir->user_handle == NULL) {
        vPortFree(dir);
        dir = NULL;
    }

    return dir;
}

directory_entry_t *readdir(directory_handle_t *dir)
{
    xSemaphoreTake(dir->driver->mutex, portMAX_DELAY);
    directory_entry_t *entry = dir->driver->io->readdir(dir->user_handle, &dir->entry);
    xSemaphoreGive(dir->driver->mutex);
    return entry;
}

void closedir(directory_handle_t *dir)
{
    xSemaphoreTake(dir->driver->mutex, portMAX_DELAY);
    dir->driver->io->closedir(dir->user_handle);
    xSemaphoreGive(dir->driver->mutex);
    vPortFree(dir);
}

static file_io_driver_t *fs_driver_head = NULL;

file_io_driver_t *fileio_find_driver(char drive_letter)
{
    if (isalnum(drive_letter) == 0) {
        printf_ts("Invalid drive letter %c(0x%02x)\n", drive_letter, drive_letter);
        return NULL;
    }
    drive_letter = toupper(drive_letter);
    vTaskSuspendAll();
    file_io_driver_t *driver = fs_driver_head;
    while (driver != NULL) {
        if (driver->drive_letter == drive_letter) {
            break;
        }
        driver = driver->next;
    }
    xTaskResumeAll();
    if (driver == NULL) {
        printf_ts("Driver not found for drive %c\n", drive_letter);
    }
    return driver;
}

int8_t fileio_register_driver(const char drive_letter, fs_io_t *io, fs_io_ll_t *io_ll, void *arg, void *ll_arg)
{
    file_io_driver_t *driver = pvPortMalloc(sizeof(file_io_driver_t));
    file_io_driver_t *prev = NULL;
    if (driver == NULL) {
        return -1;
    }

    driver->drive_letter = toupper(drive_letter);
    driver->io = io;
    driver->io_ll = io_ll;
    driver->next = NULL;

    driver->mutex = xSemaphoreCreateBinary();
    if (driver->mutex == NULL) {
        vPortFree(driver);
        return -1;
    }
    xSemaphoreGive(driver->mutex);

    vTaskSuspendAll();

    if (fs_driver_head == NULL) {
        fs_driver_head = driver;
    } else {
        file_io_driver_t *p = fs_driver_head;
        while (p->next != NULL) {
            p = p->next;
        }
        prev = p;
        p->next = driver;
    }

    xTaskResumeAll();

    user_fs_ll_handle_t *ll_handle = driver->io_ll->init(driver, ll_arg);
    if (ll_handle == NULL) {
        printf_ts("Failed to init low-level driver\n");
        vSemaphoreDelete(driver->mutex);
        vPortFree(driver);
        if (prev != NULL) {
            prev->next = NULL;
        } else {
            fs_driver_head = NULL;
        }
        return -1;
    }
    driver->ll_handle = ll_handle;

    user_fs_handle_t *handle = driver->io->init(driver, arg);
    if (handle == NULL) {
        vSemaphoreDelete(driver->mutex);
        vPortFree(driver);
        if (prev != NULL) {
            prev->next = NULL;
        } else {
            fs_driver_head = NULL;
        }
        return -1;
    }
    driver->handle = handle;

    return 0;
}

int8_t fileio_unregister_driver(const char drive_letter)
{
    int8_t status = -1;

    const char drive_letter_upper = toupper(drive_letter);

    vTaskSuspendAll();

    file_io_driver_t *driver = fs_driver_head;
    file_io_driver_t *prev = NULL;
    while (driver != NULL) {
        if (driver->drive_letter == drive_letter_upper) {
            break;
        }
        prev = driver;
        driver = driver->next;
    }

    xTaskResumeAll();

    if (driver) {
        driver->io->deinit(driver->handle);
        driver->io_ll->deinit(driver->ll_handle);
        if (prev == NULL) {
            fs_driver_head = driver->next;
        } else {
            prev->next = driver->next;
        }

        // Sync then free
        xSemaphoreTake(driver->mutex, portMAX_DELAY);
        xSemaphoreGive(driver->mutex);
        vSemaphoreDelete(driver->mutex);
        vPortFree(driver);
        status = 0;
    }

    return status;
}
