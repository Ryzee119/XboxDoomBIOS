// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "ata.h"
#include "atapi.h"
#include "cpu.h"
#include "io.h"
#include "lock.h"
#include "pci.h"
#include <stdint.h>

atomic_flag lock;

#define outb(port, val) io_output_byte(port, val)
#define inb(port)       io_input_byte(port)
#define inw(port)       io_input_word(port)
#define outw(port, val) io_output_word(port, val)
#define outl(port, val) io_output_dword(port, val)

#define ATA_ACTIVE_MASTER 0x00
#define ATA_ACTIVE_SLAVE  0x01

struct prd_entry
{
    uint32_t base_addr;
    uint16_t byte_count;
    uint16_t flags;
} __attribute__((packed, aligned(4)));

static __inline void insw(uint16_t __port, void *__buf, unsigned long __n)
{
    __asm__ __volatile__("cld; rep; insw" : "+D"(__buf), "+c"(__n) : "d"(__port));
}

static __inline__ void outsw(uint16_t __port, const void *__buf, unsigned long __n)
{
    // Do not use REP OUTSW to transfer data. There must be a tiny delay between each OUTSW. This simple loop is enough
    uint16_t *buf = (uint16_t *)__buf;
    for (uint32_t i = 0; i < __n; i++) {
        outw(__port, buf[i]);
    }
}

static void ata_io_400ns(ata_bus_t *ata_bus)
{
    volatile uint8_t alt_status;
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    alt_status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
}

static int8_t ata_busy_wait(ata_bus_t *ata_bus)
{
    uint8_t status;
    uint32_t timeout;
    uint32_t timeout_start = system_tick();
    do {
        status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            return -1;
        }
        system_yield(0);
        timeout = system_tick() - timeout_start;
    } while ((status & ATA_STATUS_BSY) && !(status & ATA_STATUS_DRQ) && (timeout < ATA_BSY_TIMEOUT));

    return (timeout >= ATA_BSY_TIMEOUT) ? -1 : 0;
}

// This will reset both ATA devices on the bus
static void ata_bus_reset(ata_bus_t *ata_bus)
{
    uint8_t control_register = inb(ata_bus->ctrl_base);
    control_register |= ATA_CTRL_SOFTWARE_RESET;
    outb(ata_bus->ctrl_base, control_register);

    system_yield(1); // > 5us

    control_register &= ~ATA_CTRL_SOFTWARE_RESET;
    outb(ata_bus->ctrl_base, control_register);

    system_yield(2);

    ata_busy_wait(ata_bus);

    // On reset, drive is reset to master automatically, but set it explicitly
    outb(ata_bus->io_base + ATA_IO_DRIVE, ATA_IO_DRIVE_SELECT_0);
}

static void ata_set_irq_en(ata_bus_t *ata_bus, uint8_t enable)
{
    uint8_t control_register = inb(ata_bus->ctrl_base);
    if (enable) {
        control_register &= ~ATA_CTRL_NIEN;
    } else {
        control_register |= ATA_CTRL_NIEN;
    }
    outb(ata_bus->ctrl_base, control_register);
}

static int8_t ata_send_command(ata_bus_t *ata_bus, uint8_t device_index, ata_command_t *ata_command)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    const uint8_t old_drive_base = inb(ata_bus->io_base + ATA_IO_DRIVE);
    const uint8_t is_lba28 = ATA_CMD_IS_LBA28(ata_command->command);
    const uint8_t is_lba48 = ATA_CMD_IS_LBA48(ata_command->command);
    int8_t error = 0;

    // Make sure the index is valid
    device_index = (device_index) ? ATA_ACTIVE_SLAVE : ATA_ACTIVE_MASTER;

    // Build the drive select byte
    uint8_t drive_base = ATA_IO_DRIVE_SELECT_0 | (device_index << 4);
    if (is_lba28 || is_lba48) {
        drive_base |= ATA_IO_DRIVE_SELECT_LBA_MASK;
        if (is_lba28) {
            drive_base |= (ata_command->lba >> 24) & 0x0F;
        }
    }

    // Select the drive, if changed wait for it to be ready
    outb(ata_bus->io_base + ATA_IO_DRIVE, drive_base);
    if ((old_drive_base ^ drive_base) & (1 << 4)) {
        error = ata_busy_wait(ata_bus);
        if (error) {
            return error;
        }
    }

    if (is_lba48) {
        outb(ata_bus->io_base + ATA_IO_FEATURES, 0);
        outb(ata_bus->io_base + ATA_IO_SECTOR_COUNT, (uint8_t)(ata_command->sector_count >> 8) & 0xFF);
        outb(ata_bus->io_base + ATA_IO_LBA_LOW, (uint8_t)((ata_command->lba >> 24) & 0xFF));
        outb(ata_bus->io_base + ATA_IO_LBA_MID, (uint8_t)((ata_command->lba >> 32) & 0xFF));
        outb(ata_bus->io_base + ATA_IO_LBA_HIGH, (uint8_t)((ata_command->lba >> 40) & 0xFF));
    }

#if (0)
    printf("[ATA] ATA_IO_DRIVE %02x\n", drive_base);
    printf("[ATA] FEATURE %02x\n", ata_command->feature);
    printf("[ATA] SECTOR_COUNT %d\n", ata_command->sector_count);
    printf("[ATA] ATA_IO_LBA_LOW %02x\n", (uint8_t)((lba >> 0) & 0xFF));
    printf("[ATA] ATA_IO_LBA_MID %02x\n", (uint8_t)((lba >> 8) & 0xFF));
    printf("[ATA] ATA_IO_LBA_HIGH %02x\n", (uint8_t)((lba >> 16) & 0xFF));
    printf("[ATA] ATA_IO_COMMAND %02x\n", ata_command->command);
#endif

    outb(ata_bus->io_base + ATA_IO_FEATURES, ata_command->feature);
    outb(ata_bus->io_base + ATA_IO_SECTOR_COUNT, ata_command->sector_count & 0xFF);
    outb(ata_bus->io_base + ATA_IO_LBA_LOW, (uint8_t)((ata_command->lba >> 0) & 0xFF));
    outb(ata_bus->io_base + ATA_IO_LBA_MID, (uint8_t)((ata_command->lba >> 8) & 0xFF));
    outb(ata_bus->io_base + ATA_IO_LBA_HIGH, (uint8_t)((ata_command->lba >> 16) & 0xFF));

    outb(ata_bus->io_base + ATA_IO_COMMAND, ata_command->command);
    ata_io_400ns(ata_bus);

    if (inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS) & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
        return -1;
    }

    return ata_busy_wait(ata_bus);
}

static int8_t atapi_send_command(ata_bus_t *ata_bus, uint8_t device_index, ata_command_t *ata_command,
                                 uint8_t atapi_command[ATAPI_CMD_SIZE])
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    int8_t error = 0;
    if (ide_device->is_present == 0) {
        return -1;
    }
    error = ata_send_command(ata_bus, device_index, ata_command);
    if (error) {
        goto bail_out;
    }

    // Send the atapi command
    outsw(ata_bus->io_base + ATA_IO_DATA, atapi_command, ATAPI_CMD_SIZE / 2);

    // Wait for the drive to be ready
    error = ata_busy_wait(ata_bus);
    if (error) {
        goto bail_out;
    }

    // Check for errors
    uint8_t status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
        error = -1;
    }

bail_out:
    return error;
}

static int8_t busmaster_dma_transfer(ata_bus_t *ata_bus, void *buffer, uint8_t read, uint32_t bytes_to_transfer)
{
    int8_t error = 0;

    // Set up the PRD Table chain, each entry can transfer up to 64KB, we allow queueing up to 8 entries (512K)
    static struct prd_entry prd_table[ATA_MAX_DMA_QUEUE_BYTES / 0xFFFF];
    uint32_t bytes_to_process = bytes_to_transfer;
    uint8_t *buffer8 = (uint8_t *)buffer;
    uint8_t prd_index = 0;

    // FIXME: This assumes contigous memory but we can scatter gather on page boundaries - although we're not paging
    // anyway so all good
    while (bytes_to_process > 0) {
        uint32_t bytes = (bytes_to_process > 0xFFFF) ? 0xFFFF : bytes_to_process;

        // Make sure we dont cross a 64K boundary
        if (((uint32_t)buffer8 & 0xFFFF) + bytes > 0x10000) {
            bytes = 0x10000 - ((uint32_t)buffer8 & 0xFFFF);
        }

        bytes_to_process -= bytes;

        prd_table[prd_index].base_addr = (uint32_t)system_get_physical_address(buffer8);
        assert((prd_table[prd_index].base_addr & 0x3) == 0);
        prd_table[prd_index].byte_count = bytes;
        prd_table[prd_index].flags = (bytes_to_process) ? 0x0000 : 0x8000; // Set MSB to indicate the last entry

        prd_index++;
        buffer8 += bytes;
    }

    outl(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_PRDT_ADDRESS, (uint32_t)prd_table);

    // Reset the busmaster controller
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND, 0x00);

    // Clear error & interrupt flags
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS,
         ATA_BUSMASTER_DMA_STATUS_INTERRUPT | ATA_BUSMASTER_DMA_STATUS_ERROR);

    // Start the DMA transfer
    uint8_t command =
        ((read) ? ATA_BUSMASTER_DMA_COMMAND_READ : ATA_BUSMASTER_DMA_COMMAND_WRITE) | ATA_BUSMASTER_DMA_COMMAND_START;
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND, command);

    // Wait for the transfer to complete (timeout 5s + 100ms per MB)
    const uint32_t timeout_time = 5000 + (bytes_to_transfer / 1024) * 100;
    const uint32_t timeout_start = system_tick();
    uint32_t timeout = 0;
    do {
        uint8_t dma_status = inb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS);
        if (!(dma_status & ATA_BUSMASTER_DMA_STATUS_ACTIVE)) {
            break;
        }
        system_yield(0);
        timeout = system_tick() - timeout_start;
    } while (timeout < timeout_time);

    // Make sure the DMA transfer is stopped
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_COMMAND, 0);

    // Check for timeout or transfer errors
    uint8_t dma_status = inb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS);
    if (timeout >= timeout_time) {
        error = -1;
    } else {
        if (dma_status & ATA_BUSMASTER_DMA_STATUS_ERROR) {
            error = -1;
        }
    }

    // Clear the interrupt flag and error flag
    outb(ata_bus->busmaster_base + ATA_BUSMASTER_DMA_STATUS,
         ATA_BUSMASTER_DMA_STATUS_INTERRUPT | ATA_BUSMASTER_DMA_STATUS_ERROR);

    // Reading this byte may perform a necessary final cache flush of the DMA data to memory.
    uint8_t device_status = inb(ata_bus->io_base + ATA_IO_STATUS);

    return error;
}

static int8_t atapi_dma_transfer(ata_bus_t *ata_bus, uint8_t device_index, void *atapi_command, uint8_t read,
                                 void *buffer, uint32_t buffer_length)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0 || ide_device->is_atapi == 0) {
        return -1;
    }

    int8_t error = 0;
    spinlock_acquire(&lock);

    ata_command_t ata_command = {
        .command = ATA_CMD_PACKET,
        .lba = ide_device->sector_size << 8, // For ATAPI, the sector size is set in the LBA mid and high field
        .sector_count = 0,
        .feature = (buffer == NULL) ? 0x00 : 0x01, // Enable DMA
    };

    error = atapi_send_command(ata_bus, device_index, &ata_command, (uint8_t *)atapi_command);
    if (error) {
        goto bail_out;
    }

    // We are done if no data is being transferred
    if (buffer == NULL) {
        goto bail_out;
    }

    ata_set_irq_en(ata_bus, 0);

    error = busmaster_dma_transfer(ata_bus, buffer, read, buffer_length);

bail_out:
    ata_set_irq_en(ata_bus, 1);
    spinlock_release(&lock);
    return error;
}

static int8_t ata_dma_transfer(ata_bus_t *ata_bus, uint8_t device_index, ata_command_t *ata_command, uint8_t read,
                               void *buffer, uint32_t buffer_length)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0 || ide_device->is_atapi == 1) {
        return -1;
    }

    int8_t error = 0;
    const int8_t is_lba28 = ATA_CMD_IS_LBA28(ata_command->command);
    const int8_t is_lba48 = ATA_CMD_IS_LBA48(ata_command->command);

    // LBA28: A sector count of 0 means 256 sectors
    // LBA48: A sector count of 0 means 65536 sectors
    uint32_t sector_count = (buffer_length + (ide_device->sector_size - 1)) / ide_device->sector_size;
    if (sector_count == 256 && is_lba28) {
        sector_count = 0;
    } else if (sector_count == 65536 && is_lba48) {
        sector_count = 0;
    }

    spinlock_acquire(&lock);

    // Need a start ata command to set the drive, lba, etc
    error = ata_send_command(ata_bus, device_index, ata_command);
    if (error) {
        goto bail_out;
    }

    ata_set_irq_en(ata_bus, 0);

    error = busmaster_dma_transfer(ata_bus, buffer, read, buffer_length);

    if (read == 0) {
        ata_command_t ata_command = {
            .command = (is_lba28) ? ATA_CMD_FLUSH_CACHE : ATA_CMD_FLUSH_CACHE_EXT,
            .lba = 0,
            .sector_count = 0,
            .feature = 0,
        };
        ata_send_command(ata_bus, 0, &ata_command);
    }

bail_out:
    ata_set_irq_en(ata_bus, 1);
    spinlock_release(&lock);
    return error;
}

static int8_t ide_dma_io(ata_bus_t *ata_bus, uint8_t device_index, uint8_t read, uint32_t lba, void *buffer,
                         uint32_t sector_count)
{
    const ide_device_t *ide_device = (device_index == 0) ? &ata_bus->master : &ata_bus->slave;
    if (ide_device->is_present == 0) {
        return -1;
    }

    if (sector_count * ide_device->sector_size > ATA_MAX_DMA_QUEUE_BYTES) {
        return -1;
    }

    if (ide_device->is_atapi) {
        atapi_read12_cmd_t atapi_command = {
            .opcode = (read) ? ATAPI_CMD_READ_12 : ATAPI_CMD_WRITE_12,
            .lba = BSWAP_BE32(lba),
            .transfer_length = BSWAP_BE32(sector_count),
            .control = 0,
            .flags = 0,
            .reserved_10 = 0,
        };
        return atapi_dma_transfer(ata_bus, device_index, &atapi_command, read, buffer,
                                  sector_count * ide_device->sector_size);
    } else {
        ata_command_t ata_command = {
            .command = (read) ? ATA_CMD_READ_LBA28_DMA : ATA_CMD_WRITE_LBA28_DMA,
            .lba = lba,
            .sector_count = sector_count,
            .feature = 0,
        };
        // LBA28 is slightly faster than LBA48, so only fallback to LBA48 if needed
        if (lba > ide_device->ata.total_sector_count_lba28 || sector_count > 256) {
            ata_command.command = (read) ? ATA_CMD_READ_LBA48_PIO : ATA_CMD_WRITE_LBA48_PIO;
        }
        return ata_dma_transfer(ata_bus, device_index, &ata_command, read, buffer,
                                sector_count * ide_device->sector_size);
    }
}

/* IDE functions atuomatically determine if ATA or ATAPI commands should be used, PIO and DMA do the same thing but you
 * probably want to use DMA */
int8_t ide_bus_init(uint16_t busmaster_base, uint16_t ctrl_base, uint16_t io_base, ata_bus_t *ata_bus)
{
    uint8_t status;
    int8_t error;

    memset(ata_bus, 0, sizeof(ata_bus_t));
    ata_bus->ctrl_base = ctrl_base;
    ata_bus->io_base = io_base;
    ata_bus->busmaster_base = busmaster_base;

    // Check for floating bus (no drives at all)
    status = inb(ata_bus->ctrl_base + ATA_CTRL_ALT_STATUS);
    if (status == 0xFF) {
        return -1;
    }

    ata_bus_reset(ata_bus);

    for (uint8_t i = 0; i < 2; i++) {
        ide_device_t *ide_device = (i == 0) ? &ata_bus->master : &ata_bus->slave;
        memset(ide_device, 0, sizeof(ide_device_t));
        ide_device->sector_size = ATA_SECTOR_SIZE;

        ata_set_irq_en(ata_bus, 0);

        // Check for ATA device, if not present, check for ATAPI
        ata_command_t cmd = {0};
        cmd.command = ATA_CMD_IDENTIFY;
        error = ata_send_command(ata_bus, i, &cmd);
        if (error) {
            ide_device->sector_size = ATAPI_SECTOR_SIZE;

            cmd.command = ATA_CMD_PACKET_IDENTIFY;
            error = ata_send_command(ata_bus, i, &cmd);
            if (error) {
                ata_set_irq_en(ata_bus, 1);
                continue;
            }

            ide_device->is_atapi = 1;
            ide_device->atapi.total_sector_count = 0;
        }

        error = ata_busy_wait(ata_bus);
        if (error) {
            ata_set_irq_en(ata_bus, 1);
            continue;
        }

        ide_device->is_present = 1;

        // Read in the identify data and pull out some key information
        for (uint16_t j = 0; j < 256; j++) {
            uint16_t wtemp = inw(ata_bus->io_base + ATA_IO_DATA);
            uint8_t *btemp = (uint8_t *)&wtemp;

            // UDMA information
            if (j == 88) {
                ide_device->selected_udma_mode = (wtemp >> 8) & 0xFF;
                ide_device->supported_udma_mode = (wtemp & 0xFF);
            }

            if (ide_device->is_atapi == 0) {
                // 80 wire conductor information
                // it 13 of word 93 is CBLID, it is pulled low on 80 wire conductors, otherwise high
                if (j == 93) {
                    if (!(wtemp & (1 << 13))) {
                        ata_bus->wire80 = 1;
                    } else {
                        ide_device->supported_udma_mode &= 0x03; // Restrict to UDMA 2 or less of 40 wire conductor
                    }

                    //Xbox specific. Apparently the 80 wire conduct bit is not reliable on xbox, so we assume it is low always
                    ide_device->supported_udma_mode &= 0x03;

                    // We atleast need UDMA1. This shouldnt be zero
                    if (ide_device->supported_udma_mode == 0) {
                        ide_device->supported_udma_mode = 0x01;
                    }
                }

                // LBA28 Addressing
                else if (j == 60) {
                    ide_device->ata.total_sector_count_lba28 = wtemp;
                } else if (j == 61) {
                    ide_device->ata.total_sector_count_lba28 |= wtemp << 16;

                    ide_device->ata.total_sector_count_lba28 =
                        BSWAP_LE32_TO_NATIVE(ide_device->ata.total_sector_count_lba28 + 1);
                }

                // LBA48 Addressing
                else if (j == 100) {
                    ide_device->ata.total_sector_count_lba48 = wtemp;
                } else if (j == 101) {
                    ide_device->ata.total_sector_count_lba48 |= wtemp << 16;
                } else if (j == 102) {
                    ide_device->ata.total_sector_count_lba48 |= (uint64_t)wtemp << 32;
                } else if (j == 103) {
                    ide_device->ata.total_sector_count_lba48 |= (uint64_t)wtemp << 48;

                    ide_device->ata.total_sector_count_lba48 =
                        BSWAP_LE64_TO_NATIVE(ide_device->ata.total_sector_count_lba48 + 1);
                }
            }

            // Model, Serial, Firmware
            if (j >= 27 && j < 47) {
                uint8_t k = (j - 27) * 2;
                ide_device->model[k] = btemp[1];
                ide_device->model[k + 1] = btemp[0];
            } else if (j >= 10 && j < 20) {
                uint8_t k = (j - 10) * 2;
                ide_device->serial[k] = btemp[1];
                ide_device->serial[k + 1] = btemp[0];
            } else if (j >= 23 && j < 27) {
                uint8_t k = (j - 23) * 2;
                ide_device->firmware[k] = btemp[1];
                ide_device->firmware[k + 1] = btemp[0];
            }
        }

        // Enable fastest UDMA mode supported
        // Section 6.48 T13/1532D Volume 1 Revision 1a Table 41
        if (ide_device->supported_udma_mode) {
            uint8_t udma_mode = 0x00;
            for (int8_t mode = 6; mode >= 0; mode--) {
                if (ide_device->supported_udma_mode & (1 << (mode - 1))) {
                    udma_mode = ATA_TRANSFER_MODE_UDMA | mode;
                    break;
                }
            }
            udma_mode = ATA_TRANSFER_MODE_UDMA | udma_mode;
            printf("[ATA] Setting UDMA Mode %02x\n", udma_mode);

            outb(ata_bus->io_base + ATA_IO_FEATURES, ATA_FEATURE_SET_TRANSFER_MODE);
            outb(ata_bus->io_base + ATA_IO_SECTOR_COUNT, udma_mode);
            outb(ata_bus->io_base + ATA_IO_COMMAND, ATA_CMD_SET_FEATURES);
            ata_busy_wait(ata_bus);
        }
        ata_set_irq_en(ata_bus, 1);

#if (1)
        if (ide_device->is_present) {
            printf("\n[ATA] Mode: %s\n", ide_device->model);
            printf("[ATA] Serial: %s\n", ide_device->serial);
            printf("[ATA] Firmware: %s\n", ide_device->firmware);
            printf("[ATA] Wire80: %d\n", ata_bus->wire80);
            printf("[ATA] Supported UDMA: %02x\n", ide_device->supported_udma_mode);
            if (ide_device->is_atapi == 0) {
                printf("[ATA] LBA28 Sectors: %d\n", ide_device->ata.total_sector_count_lba28);
                printf("[ATA] LBA48 Sectors: %llu\n", ide_device->ata.total_sector_count_lba48);
            } else {
                printf("[ATAPI] Total Sectors: %llu\n", ide_device->atapi.total_sector_count);
            }
        }
#endif

#if (0)
        if (ide_device->is_atapi) {
            system_yield(1000);
            atapi_read_capacity_cmd_t atapi_command_buffer;
            atapi_read_capacity_response_t atapi_read_capacity_response;
            printf("[ATAPI] Device found on %d\n", i);

            // Temporarily set the sector size to 8 bytes because we don't know the real size yet
            ide_device->sector_size = 8;

            memset(&atapi_command_buffer, 0, ATAPI_CMD_SIZE);
            atapi_command_buffer.opcode = ATAPI_CMD_READ_CAPACITY;

            error = atapi_dma_transfer(ata_bus, i, &atapi_command_buffer, 1, &atapi_read_capacity_response,
                                       sizeof(atapi_read_capacity_response));
            if (error) {
                printf("[ATAPI] Error reading capacity\n");
                continue;
            }

            ide_device->sector_size = BSWAP_BE32_TO_NATIVE(atapi_read_capacity_response.block_size);
            ide_device->atapi.total_sector_count = BSWAP_BE32_TO_NATIVE(atapi_read_capacity_response.last_lba) + 1;

            printf("[ATAPI] Sector Size: %d, Last LBA %llu\n\n\n", ide_device->sector_size,
                   ide_device->atapi.total_sector_count);
        }
#endif
    }

    ata_bus_reset(ata_bus);
    return error;
}

// For DMA, the data buffers cannot cross a 64K boundary, and must be contiguous in physical memory
int8_t ide_dma_read(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, void *buffer, uint32_t sector_count)
{
    // Ensure 4 bytes algined
    assert(((uint32_t)buffer & 0x3) == 0);
    return ide_dma_io(ata_bus, device_index, 1, lba, buffer, sector_count);
}

// For DMA, the data buffers cannot cross a 64K boundary, and must be contiguous in physical memory
int8_t ide_dma_write(ata_bus_t *ata_bus, uint8_t device_index, uint32_t lba, const void *buffer, uint32_t sector_count)
{
    assert(((uint32_t)buffer & 0x3) == 0);
    return ide_dma_io(ata_bus, device_index, 0, lba, (void *)buffer, sector_count);
}
