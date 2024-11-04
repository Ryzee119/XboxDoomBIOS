// SPDX-License-Identifier: CC0-1.0

#ifndef ATA_H
#define ATA_H

#include "lock.h"
#include <stdint.h>

#define ATA_BUSMASTER_DMA_COMMAND      0x00
#define ATA_BUSMASTER_DMA_STATUS       0x02
#define ATA_BUSMASTER_DMA_PRDT_ADDRESS 0x04

#define ATA_BUSMASTER_DMA_COMMAND_START (1 << 0)
#define ATA_BUSMASTER_DMA_COMMAND_READ  (1 << 3)
#define ATA_BUSMASTER_DMA_COMMAND_WRITE (0 << 3)

#define ATA_BUSMASTER_DMA_STATUS_ACTIVE    (1 << 0)
#define ATA_BUSMASTER_DMA_STATUS_ERROR     (1 << 1)
#define ATA_BUSMASTER_DMA_STATUS_INTERRUPT (1 << 2)

#define ATA_IO_DATA         0
#define ATA_IO_ERROR        1
#define ATA_IO_FEATURES     1
#define ATA_IO_SECTOR_COUNT 2
#define ATA_IO_LBA_LOW      3
#define ATA_IO_LBA_MID      4
#define ATA_IO_LBA_HIGH     5
#define ATA_IO_DRIVE        6
#define ATA_IO_COMMAND      7
#define ATA_IO_STATUS       7

#define ATA_IO_DRIVE_SELECT_LBA_MASK 0x40
#define ATA_IO_DRIVE_SELECT_0        0xA0

#define ATA_CTRL_HOB            (1 << 3)
#define ATA_CTRL_SOFTWARE_RESET (1 << 2)
#define ATA_CTRL_NIEN           (1 << 1)

#define ATA_CTRL_ALT_STATUS     0
#define ATA_CTRL_DEVICE_CONTROL 0

#define ATA_CMD_READ_LBA28_DMA  0xC8
#define ATA_CMD_WRITE_LBA28_DMA 0xCA
#define ATA_CMD_READ_LBA28_PIO  0x20
#define ATA_CMD_WRITE_LBA28_PIO 0x30

#define ATA_CMD_READ_LBA48_DMA  0x25
#define ATA_CMD_WRITE_LBA48_DMA 0x35
#define ATA_CMD_READ_LBA48_PIO  0x24
#define ATA_CMD_WRITE_LBA48_PIO 0x34

#define ATA_CMD_IS_LBA48(cmd) (cmd == 0x25 || cmd == 0x35 || cmd == 0x24 || cmd == 0x34)
#define ATA_CMD_IS_LBA28(cmd) (cmd == 0xC8 || cmd == 0xCA || cmd == 0x20 || cmd == 0x30)
#define ATA_CMD_IS_LBA(cmd)   (ATA_CMD_IS_LBA28(cmd) || ATA_CMD_IS_LBA48(cmd))

#define ATA_CMD_FLUSH_CACHE     0xE7
#define ATA_CMD_FLUSH_CACHE_EXT 0xEA
#define ATA_CMD_IDENTIFY        0xEC
#define ATA_CMD_PACKET_IDENTIFY 0xA1
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_SET_FEATURES    0xEF

#define ATA_FEATURE_DMA 0x01
#define ATA_FEATURE_SET_TRANSFER_MODE  0x03
#define ATA_TRANSFER_MODE_PIO          0x00
#define ATA_TRANSFER_MODE_PIO_NO_IORDY 0x01
#define ATA_TRANSFER_MODE_PIO_MODE_3   0x0B
#define ATA_TRANSFER_MODE_PIO_MODE_4   0x0C
#define ATA_TRANSFER_MODE_UDMA         0x40 // Or with the mode number

#define ATA_STATUS_ERR (1 << 0) // Indicates an error occurred. Send a new command to clear it
#define ATA_STATUS_DRQ (1 << 3) // Set when the drive has PIO data to transfer, or is ready to accept PIO data.
#define ATA_STATUS_SRV                                                                                                 \
    (1 << 4) // Overlapped Mode Service Request. This bit is set when the drive requires service in an overlapped
             // operation.
#define ATA_STATUS_DF  (1 << 5) // Drive Fault. Indicates that a fault occurred during a drive operation.
#define ATA_STATUS_RDY (1 << 6) // Bit is clear when drive is spun down, or after an error. Set otherwise.
#define ATA_STATUS_BSY (1 << 7) // Indicates the drive is preparing to send/receive data (wait for it to clear).

#define ATA_SECTOR_SIZE   512
#define ATAPI_SECTOR_SIZE 2048

#ifndef ATA_BSY_TIMEOUT
#define ATA_BSY_TIMEOUT 10000
#endif

#ifndef ATA_MAX_DMA_QUEUE_BYTES
#define ATA_MAX_DMA_QUEUE_BYTES (65536 * 8) // Must be a multiple of 65536
#endif

typedef struct ata_command
{
    uint8_t command;
    uint8_t feature;
    uint16_t sector_count;
    uint64_t lba;
} ata_command_t;

typedef struct ide_device
{
    uint8_t is_present : 1;
    uint8_t is_atapi : 1;
    uint8_t reserved : 6;
    uint8_t supported_udma_mode;
    uint8_t selected_udma_mode;
    uint16_t sector_size;

    union
    {
        struct
        {
            uint32_t total_sector_count_lba28;
            uint64_t total_sector_count_lba48;
        } ata;

        struct
        {
            uint64_t total_sector_count;
            uint8_t reserved[2];
        } atapi;
    };

    uint8_t model[41];
    uint8_t serial[21];
    uint8_t firmware[9];
} ide_device_t;

typedef struct ata_bus
{
    uint8_t wire80 : 1;
    uint8_t reserved : 7;

    uint16_t ctrl_base;
    uint16_t io_base;
    uint16_t busmaster_base;

    ide_device_t master;
    ide_device_t slave;
    atomic_flag lock;
} ata_bus_t;

int8_t ide_bus_init(uint16_t busmaster_base, uint16_t ctrl_base, uint16_t io_base, ata_bus_t *ata_bus);

// Main read and write functions for IDE bus devices. Autommatically selects ATA or ATAPI based on the device type
int8_t ide_dma_read(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, void *buffer, uint32_t sector_count);
int8_t ide_dma_write(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, const void *buffer, uint32_t sector_count);

#endif // ATA_H
