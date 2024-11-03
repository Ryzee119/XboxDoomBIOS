#include "main.h"

#define MSC_DEBUG
#ifdef MSC_DEBUG
#define MSC_PRINTF printf_ts
#else
#define MSC_PRINTF(...)
#endif

typedef struct msc_device
{
    uint8_t dev_addr;
    char drive_letter;
} msc_device_t;

static msc_device_t msc_device[CFG_TUH_MSC] = {0};

static bool usb_transfer_cb(uint8_t dev_addr, tuh_msc_complete_data_t const *cb_data)
{
    int *finished = (int *)cb_data->user_arg;
    msc_csw_t const *csw = cb_data->csw;

    if (csw->status == 0) {
        *finished = 1;
    } else {
        printf_ts("[USBMSC] SCSI command failed with status %d\n", csw->status);
        *finished = -1;
    }

    return true;
}

static void msc_mount_task(void *parameters)
{
    msc_device_t *msc = (msc_device_t *)parameters;
    char drive_letter = msc->drive_letter;

    char path[3] = {drive_letter, ':', '\0'};
    extern fs_io_ll_t usb_ll_io;
    extern fs_io_t fat_io;
    extern fs_io_t fatx_io;

    // Try fatfs first
    if (fileio_register_driver(drive_letter, &fat_io, &usb_ll_io, NULL, NULL) == 0) {
        MSC_PRINTF("[USBMSC] Mounted drive %c as FAT\n", drive_letter);
        vTaskDelete(NULL);
        return;
    }

    if (fileio_register_driver(drive_letter, &fatx_io, &usb_ll_io, NULL, NULL) == 0) {
        MSC_PRINTF("[USBMSC] Mounted drive %c as FATX\n", drive_letter);
        vTaskDelete(NULL);
        return;
    }

    MSC_PRINTF("[USBMSC] Failed to mount drive %c. No valid filesystem\n", drive_letter);
    vTaskDelete(NULL);
    return;
}

void tuh_msc_mount_cb(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr == 0) {
            msc_device[i].dev_addr = dev_addr;
            msc_device[i].drive_letter = '0' + i;
            xTaskCreate(msc_mount_task, "msc_mount_task", configMINIMAL_STACK_SIZE, &msc_device[i],
                        THREAD_PRIORITY_NORMAL, NULL);
            return;
        }
    }
    MSC_PRINTF("[USBMSC] No space to store device\r\n");
}

void tuh_msc_umount_cb(uint8_t dev_addr)
{
    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].dev_addr == dev_addr) {
            msc_device[i].dev_addr = 0;
            msc_device[i].drive_letter = 0;
            MSC_PRINTF("[USBMSC] Address %d unmounted from index %d\r\n", dev_addr, i);
            return;
        }
    }
}

static user_fs_ll_handle_t *usb_disk_init(file_io_driver_t *driver, void *arg)
{
    (void)arg;

    for (uint8_t i = 0; i < CFG_TUH_MSC; i++) {
        if (msc_device[i].drive_letter == driver->drive_letter) {
            user_fs_ll_handle_t *handle = (user_fs_ll_handle_t *)&msc_device[i];
            return handle;
        }
    }

    return NULL;
}

static void usb_disk_deinit(user_fs_ll_handle_t *handle)
{
    msc_device_t *msc = (msc_device_t *)handle;
    msc->dev_addr = 0;
    msc->drive_letter = 0;
    return;
}

static int8_t usb_disk_read(user_fs_ll_handle_t *handle, void *buffer, uint64_t sector_offset, size_t sector_count)
{
    msc_device_t *msc = (msc_device_t *)handle;
    char drive_letter = msc->drive_letter;

    int finished = 0;
    if (tuh_msc_read10(msc->dev_addr, 0, buffer, sector_offset, (uint16_t)sector_count, usb_transfer_cb,
                       (uintptr_t)&finished)) {
        while (finished == 0) {
            system_yield(0);
        }
        if (finished == -1) {
            printf_ts("disk_read failed, sector %d, count %d\n", sector_offset, sector_count);
            return -1;
        }
        return 0;
    }
    return -1;
}

static int8_t usb_disk_write(user_fs_ll_handle_t *handle, const void *buffer, uint64_t sector_offset,
                              size_t sector_count)
{
    msc_device_t *msc = (msc_device_t *)handle;
    char drive_letter = msc->drive_letter;

    int finished = 0;
    if (tuh_msc_write10(msc->dev_addr, 0, buffer, sector_offset, (uint16_t)sector_count, usb_transfer_cb,
                        (uintptr_t)&finished)) {
        while (finished == 0) {
            system_yield(0);
        }
        if (finished == -1) {
            printf_ts("disk_write failed\n");
            return -1;
        }
        return 0;
    }
    return -1;
}

static int8_t usb_disk_ioctl(user_fs_ll_handle_t *handle, fs_ioctrl_cmd_t cmd, void *buff)
{
    msc_device_t *msc = (msc_device_t *)handle;
    char drive_letter = msc->drive_letter;

    switch (cmd) {
        case FS_IO_SYNC:
            return 0;
        case FS_IO_GET_SECTOR_COUNT:
            *((uint64_t *)buff) = (uint64_t)tuh_msc_get_block_count(msc->dev_addr, 0);
            return 0;
        case FS_IO_GET_SECTOR_SIZE:
            *((uint16_t *)buff) = (uint16_t)tuh_msc_get_block_size(msc->dev_addr, 0);
            return 0;
        case FS_IO_GET_BLOCK_SIZE:
            *((uint16_t *)buff) = 1;
            return 0;
        default:
            return -1;
    }
}

fs_io_ll_t usb_ll_io = {
    .init = usb_disk_init,
    .deinit = usb_disk_deinit,
    .read = usb_disk_read,
    .write = usb_disk_write,
    .ioctrl = usb_disk_ioctl,
};
