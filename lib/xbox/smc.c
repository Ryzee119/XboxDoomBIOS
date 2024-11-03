// SPDX-License-Identifier: CC0-1.0
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

int8_t xbox_smc_get_tray_state(xbox_tray_state_t *tray_state)
{
    return smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_TRAY_STATE, tray_state);
}

int8_t xbox_smc_get_avpack(xbox_av_pack_t *av_pack)
{
    return smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_AVPACK, av_pack);
}

int8_t xbox_smc_set_tray_state(xbox_tray_control_t tray_control)
{
    return smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_TRAY_CLOSED, tray_control);
}

int8_t xbox_smc_set_power(xbox_power_control_t power)
{
    return smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_POWER, power);
}

int8_t xbox_smc_get_version(uint8_t version[4])
{
    memset(version, 0, 4);

    // Reset counter
    if (smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_VERSION, 0) < 0) {
        return -1;
    }

    // Read 3 bytes
    for (uint8_t i = 0; i < 3; i++) {
        if  (smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_VERSION, &version[i]) < 0) {
            return -1;
        }
    }

    return 0;
}

int8_t xbox_smc_set_fan(uint8_t fan_percent)
{
    // Return to auto mode
    if (fan_percent == 0) {
        return smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_FAN_MODE, 0);
    }

    // 0 - 50 = 0 - 100%
    fan_percent /= 2;

    if (smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_FAN_MODE, 1) > 0) {
        return smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_SET_FAN_SPEED, fan_percent);
    }
    return -1;
}

int8_t xbox_smc_read_temperatures(uint8_t *cpu_temp, uint8_t *mb_temp)
{
    int8_t status = 0;
    if (smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_CPU_TEMP, cpu_temp) < 0) {
        *cpu_temp = 0;
        status = -1;
    }
    
    if (smbus_input_byte(XBOX_SMBUS_ADDRESS_SMC, XBOX_SMC_GET_MB_TEMP, mb_temp)) {
        *mb_temp = 0;
        status = -1;
    }
    return status;
}
