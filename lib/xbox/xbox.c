// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "xbox.h"

// power
// tray
// fan
// reboot

void xbox_interrupt_enable(uint8_t irq, uint8_t enable)
{
    uint8_t data_port = (irq < 8) ? XBOX_PIC1_DATA_PORT : XBOX_PIC2_DATA_PORT;
    if (irq >= 8) {
        irq -= 8;
    }
    if (enable) {
        pic8259_irq_enable(data_port, irq);
    } else {
        pic8259_irq_disable(data_port, irq);
    }
}
