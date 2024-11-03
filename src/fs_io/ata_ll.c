#include "main.h"

typedef struct ata_ll_data
{
    ata_bus_t *ata_bus;
    uint8_t drive_index;
} ata_ll_data_t;

static user_fs_ll_handle_t *ata_disk_init(file_io_driver_t *driver, void *arg)
{
    char drive_letter = toupper(driver->drive_letter);

    user_fs_ll_handle_t *handle = pvPortMalloc(sizeof(ata_ll_data_t));
    if (handle == NULL) {
        return NULL;
    }

    // First half of the alphabet will use drive index 0, second half will use drive index 1
    // Except D which is always 1 and X, Y, Z which are always 0
    // Drive index 0 is master on IDE cable, index 1 is slave
    uint8_t drive_index;
    if (drive_letter == 'D') {
        drive_index = 1;
    } else if (drive_letter == 'X' || drive_letter == 'Y' || drive_letter == 'Z' || drive_letter <= 'M') {
        drive_index = 0;
    } else {
        drive_index = 1;
    }

    ata_ll_data_t *ata_ll_data = (ata_ll_data_t *)handle;
    ata_ll_data->drive_index = drive_index;
    ata_ll_data->ata_bus = (ata_bus_t *)arg;
    return handle;
}

static void ata_disk_deinit(user_fs_ll_handle_t *handle)
{
    vPortFree(handle);
    return;
}

static int8_t ata_disk_read(user_fs_ll_handle_t *handle, void *buffer, uint64_t sector_offset, size_t sector_count)
{
    ata_ll_data_t *ata_ll_data = (ata_ll_data_t *)handle;
    return ide_dma_read(ata_ll_data->ata_bus, ata_ll_data->drive_index, sector_offset, buffer, sector_count);
}

static int8_t ata_disk_write(user_fs_ll_handle_t *handle, const void *buffer, uint64_t sector_offset,
                              size_t sector_count)
{
    ata_ll_data_t *ata_ll_data = (ata_ll_data_t *)handle;
    return ide_dma_write(ata_ll_data->ata_bus, ata_ll_data->drive_index, sector_offset, buffer, sector_count);
}

static int8_t ata_disk_ioctl(user_fs_ll_handle_t *handle, fs_ioctrl_cmd_t cmd, void *buff)
{
    ata_ll_data_t *ata_ll_data = (ata_ll_data_t *)handle;
    ata_bus_t *ata_bus = ata_ll_data->ata_bus;
    uint8_t drive_index = ata_ll_data->drive_index;

    ide_device_t *ide_device = (drive_index == 0) ? &ata_bus->master : &ata_bus->slave;

    switch (cmd) {
        case FS_IO_SYNC:
            return 0;
        case FS_IO_GET_SECTOR_COUNT:
            if (ide_device->is_atapi) {
                *((uint64_t *)buff) = ide_device->atapi.total_sector_count;
            } else {
                *((uint64_t *)buff) = MAX(ide_device->ata.total_sector_count_lba28, ide_device->ata.total_sector_count_lba48);
            }
            return 0;
        case FS_IO_GET_SECTOR_SIZE:
            *((uint16_t *)buff) = ide_device->sector_size;
            return 0;
        case FS_IO_GET_BLOCK_SIZE:
            *((uint16_t *)buff) = 1;
            return 0;
        default:
            return -1;
    }
}

fs_io_ll_t ata_ll_io = {
    .init = ata_disk_init,
    .deinit = ata_disk_deinit,
    .read = ata_disk_read,
    .write = ata_disk_write,
    .ioctrl = ata_disk_ioctl,
};
