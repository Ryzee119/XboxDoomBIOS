// SPDX-License-Identifier: CC0-1.0

#ifndef LED_H
#define LED_H

#include "xbox.h"

enum
{
    XLED_OFF = 0x00,
    XLED_GREEN = 0x01,
    XLED_RED = 0x10,
    XLED_ORANGE = 0x11
};
typedef uint8_t xbox_led_colour_t;

void xbox_led_output(xbox_led_colour_t t1, xbox_led_colour_t t2, xbox_led_colour_t t3, xbox_led_colour_t t4);
void xbox_led_reset(void);

#endif