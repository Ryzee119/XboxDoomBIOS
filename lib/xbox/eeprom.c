// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

static xbox_eeprom_t xbox_eeprom;
static uint8_t cached_eeprom = 0;

static int16_t xbox_eeprom_read()
{
    int16_t total_read = 0;
    for (uint32_t i = 0; i < 256; i++) {
        uint8_t *eeprom8 = (uint8_t *)&xbox_eeprom;
        int8_t bytes_read = smbus_input_byte(XBOX_SMBUS_ADDRESS_EEPROM, i, &eeprom8[i]);
        if (bytes_read < 0) {
            return -1;
        }
        total_read += bytes_read;
    }

    return total_read;
}

const xbox_eeprom_t *xbox_eeprom_get()
{
    if (cached_eeprom == 0) {
        if (xbox_eeprom_read() == sizeof(xbox_eeprom_t)) {
            cached_eeprom = 1;
        } else {
            return NULL;
        }
    }
    return &xbox_eeprom;
}

int16_t xbox_eeprom_set(xbox_eeprom_t *eeprom)
{
    int16_t total_written = 0;
    uint8_t *cached_eeprom8 = (uint8_t *)xbox_eeprom_get();

    for (uint32_t i = 0; i < 256; i++) {
        uint8_t *eeprom8 = (uint8_t *)eeprom;
        if (eeprom8[i] == cached_eeprom8[i]) {
            total_written += 1;
            continue;
        }

        if (smbus_output_byte(XBOX_SMBUS_ADDRESS_EEPROM, i, eeprom8[i]) < 0) {
            return -1;
        }

        cached_eeprom8[i] = eeprom8[i];
        total_written += 1;
    }

    return total_written;
}
