// SPDX-License-Identifier: CC0-1.0

#include "xbox.h"

#define LED_MODE_AUTO   0x00
#define LED_MODE_MANUAL 0x01

#define SMC_REG_LEDMODE 0x07
#define SMC_REG_LEDSEQ  0x08

void xbox_led_output(xbox_led_colour_t t1, xbox_led_colour_t t2, xbox_led_colour_t t3, xbox_led_colour_t t4)
{
    uint8_t led_colour = ((t1 & 0x11) << 3) | ((t2 & 0x11) << 2) | ((t3 & 0x11) << 1) | ((t4 & 0x11) << 0);

    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, SMC_REG_LEDMODE, LED_MODE_MANUAL);
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, SMC_REG_LEDSEQ, led_colour);
}

void xbox_led_reset(void)
{
    smbus_output_byte(XBOX_SMBUS_ADDRESS_SMC, SMC_REG_LEDMODE, LED_MODE_AUTO);
}
