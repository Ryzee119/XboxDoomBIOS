// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include <stdint.h>
#include "io.h"
#include "lock.h"
#include "smbus.h"

// https://xboxdevwiki.net/SMBus

static atomic_flag lock;
static uint16_t SMBUS_IO_BASE = 0xC000;

void smbus_init(uint16_t io_base) {
    SMBUS_IO_BASE = io_base;
}

static int8_t smbus_io(uint8_t address, uint8_t command, void *data, uint8_t data_len, uint8_t read)
{
    uint16_t status;
    uint32_t actual_length = data_len;
    uint8_t *data8 = (uint8_t *)data;
    uint16_t *data16 = (uint16_t *)data;

    if (read) {
        read = 1;
    }

smbus_collision_retry:
    spinlock_acquire(&lock);

    while (io_input_word(SMBUS_STATUS) & SMBUS_STATUS_BUSY) {
        system_yield(0);
    }

    io_output_byte(SMBUS_ADDRESS, address | read);

    if (data_len > 0) {
        io_output_byte(SMBUS_COMMAND, command);
    }

    io_output_word(SMBUS_STATUS, 0xFFFF);

    uint8_t transfer_type;
    switch (data_len) {
        case 0:
            transfer_type = SMBUS_CONTROL_TRANSFER_TYPE_ZERO;
            break;
        case 1:
            transfer_type = SMBUS_CONTROL_TRANSFER_TYPE_BYTE;
            if (read == 0) {
                io_output_byte(SMBUS_DATA, *data8);
            }
            break;
        case 2:
            transfer_type = SMBUS_CONTROL_TRANSFER_TYPE_WORD;
            if (read == 0) {
                io_output_word(SMBUS_DATA, *data16);
            }
            break;
        default:
            transfer_type = SMBUS_CONTROL_TRANSFER_TYPE_BLOCK;
            if (read == 0) {
                io_output_byte(SMBUS_DATA, data_len);
                for (uint8_t i = 0; i < data_len; i++) {
                    io_output_byte(SMBUS_BLOCK_DATA, data8[i]);
                }
            }
            break;
    }

    // Start the transfer
    io_output_byte(SMBUS_CONTROL, transfer_type | SMBUS_CONTROL_START);

    // Wait for completion
    while (io_input_word(SMBUS_STATUS) & SMBUS_STATUS_BUSY) {
        system_yield(1);
    }

    // Get the transfer status
    status = io_input_word(SMBUS_STATUS);
    io_output_word(SMBUS_STATUS, 0xFFFF);

    if (status & SMBUS_STATUS_ERROR) {
        spinlock_release(&lock);
        if (status & SMBUS_STATUS_COLLISION) {
            goto smbus_collision_retry;
        }
#if (0)
        printf("[SMBUS] ERROR Address %02x, Command: %02x Error: %04X: ", address, command, status);
        if (status & SMBUS_STATUS_ABORT) {
            printf("Aborted ");
        }
        if (status & SMBUS_STATUS_PROTOCOL_ERROR) {
            printf("Protocol Error ");
        }
        if (status & SMBUS_STATUS_TIMEOUT) {
            printf("Timeout ");
        }
        printf("\n");
#endif
        return SMBUS_RETURN_ERROR;
    }

    if (read) {
        switch (data_len) {
            case 1:
                *data8 = io_input_byte(SMBUS_DATA);
                break;
            case 2:
                *data16 = io_input_word(SMBUS_DATA);
                break;
            default:
                actual_length = io_input_byte(SMBUS_DATA);
                if (actual_length > data_len) {
                    actual_length = data_len;
                }
                for (uint8_t i = 0; i < actual_length; i++) {
                    data8[i] = io_input_byte(SMBUS_BLOCK_DATA);
                }
                break;
        }
    }
    spinlock_release(&lock);
    return actual_length;
}

int8_t smbus_poke(uint8_t address)
{
    return smbus_io(address, 0, NULL, 0, 0);
}

int8_t smbus_input_byte(uint8_t address, uint8_t reg, uint8_t *data)
{
    return smbus_io(address, reg, data, 1, 1);
}

int8_t smbus_input_word(uint8_t address, uint8_t reg, uint16_t *data)
{
    return smbus_io(address, reg, data, 2, 1);
}

int8_t smbus_input_dword(uint8_t address, uint8_t reg, uint32_t *data)
{
    return smbus_io(address, reg, data, 4, 1);
}

int8_t smbus_output_byte(uint8_t address, uint8_t reg, uint8_t data)
{
    return smbus_io(address, reg, &data, 1, 0);
}

int8_t smbus_output_word(uint8_t address, uint8_t reg, uint16_t data)
{
    return smbus_io(address, reg, &data, 2, 0);
}

int8_t smbus_output_dword(uint8_t address, uint8_t reg, uint32_t data)
{
    return smbus_io(address, reg, &data, 4, 0);
}