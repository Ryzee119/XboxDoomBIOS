// SPDX-License-Identifier: CC0-1.0
// Copyright (c) 2024 Ryzee119

#ifndef SMC_H
#define SMC_H

#include "xbox.h"

//  Each time you read that register, the next of the 3 characters is returned. The counter can be reset to the first
//  letter by writing 0x00 to this register
#define XBOX_SMC_GET_VERSION    0x01 // 3 ASCII chars
#define XBOX_SMC_GET_TRAY_STATE 0x03
#define XBOX_SMC_GET_AVPACK     0x04
#define XBOX_SMC_GET_CPU_TEMP   0x09 // Celcius
#define XBOX_SMC_GET_MB_TEMP    0x0A // Celcius
#define XBOX_SMC_GET_FAN_SPEED  0x10 // 0 to 50 = 0 to 100%
#define XBOX_SMC_GET_IRQ        0x11

#define XBOX_SMC_SET_POWER       0x02 // 0x01 = reset, 0x40 - power cycle, 0x80 - power off
#define XBOX_SMC_SET_FAN_MODE    0x05 // 0 = auto, 1 = manual
#define XBOX_SMC_SET_FAN_SPEED   0x06 // 0 to 50 = 0 to 100%
#define XBOX_SMC_SET_TRAY_CLOSED 0x0C // 0 = open, 1 = load
#define XBOX_SMC_SET_IRQ_EN      0x1A // 1 = enable. Cannot disable

enum xbox_power_control
{
    XBOX_SMC_SET_PWR_RESET = 0x01,
    XBOX_SMC_SET_PWR_POWER_CYCLE = 0x40,
    XBOX_SMC_SET_PWR_POWER_OFF = 0x80,
};
typedef uint8_t xbox_power_control_t;

enum xbox_tray_control
{
    XBOX_SMC_TRAY_CONTROL_OPEN = 0x00,
    XBOX_SMC_TRAY_CONTROL_CLOSE = 0x01,
};
typedef uint8_t xbox_tray_control_t;

enum xbox_av_pack
{
    XBOX_AV_PACK_SCART = 0x00,
    XBOX_AV_PACK_HDTV = 0x01,
    XBOX_AV_PACK_VGA = 0x02,
    XBOX_AV_PACK_RFU = 0x03,
    XBOX_AV_PACK_SVIDEO = 0x04,
    XBOX_AV_PACK_UNDEFINED = 0x05,
    XBOX_AV_PACK_STANDARD = 0x06,
    XBOX_AV_PACK_MISSING = 0x07,
};
typedef uint8_t xbox_av_pack_t;

enum xbox_tray_state
{
    XBOX_TRAY_STATE_BUSY = 0x01,
    XBOX_TRAY_STATE_CLOSED = 0x00,
    XBOX_TRAY_STATE_OPEN = 0x10,
    XBOX_TRAY_STATE_UNLOADING = 0x20,
    XBOX_TRAY_STATE_OPENING = 0x30,
    XBOX_TRAY_STATE_EMPTY = 0x40,
    XBOX_TRAY_STATE_CLOSING = 0x50,
    XBOX_TRAY_STATE_DVD_DETECTED = 0x60,
    XBOX_TRAY_STATE_RESET = 0x70,
};
typedef uint8_t xbox_tray_state_t;

int8_t xbox_smc_get_tray_state(xbox_tray_state_t *tray_state);
int8_t xbox_smc_get_avpack(xbox_av_pack_t *av_pack);
int8_t xbox_smc_set_tray_state(xbox_tray_control_t tray_control);
int8_t xbox_smc_set_power(xbox_power_control_t power);
int8_t xbox_smc_get_version(uint8_t version[4]);
int8_t xbox_smc_set_fan(uint8_t fan_percent);
int8_t xbox_smc_read_temperatures(uint8_t *cpu_temp, uint8_t *mb_temp);
#endif