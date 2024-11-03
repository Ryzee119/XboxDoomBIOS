// SPDX-License-Identifier: CC0-1.0

#ifndef PIT_H
#define PIT_H

#include <stdint.h>

// PIT operating modes
#define PIT_MODE_TERMINAL_COUNT  0x00
#define PIT_MODE_ONE_SHOT        0x02
#define PIT_MODE_RATE_GENERATOR  0x04
#define PIT_MODE_SQUARE_WAVE     0x06
#define PIT_MODE_SOFTWARE_STROBE 0x08
#define PIT_MODE_HARDWARE_STROBE 0x0A

// PIT access modes
#define PIT_ACCESS_LATCH_COUNT 0x00
#define PIT_ACCESS_LOBYTE      0x10
#define PIT_ACCESS_HIBYTE      0x20
#define PIT_ACCESS_LOHIBYTE    0x30

#endif // PIT_H