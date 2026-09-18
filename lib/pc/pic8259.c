// SPDX-License-Identifier: CC0-1.0

#include <stdint.h>

#include "io.h"
#include "pic8259.h"

void pic8259_irq_enable(uint8_t data_port, uint8_t irq)
{
    uint32_t flags;
    __asm volatile("pushf; pop %0; cli" : "=r"(flags));
    uint8_t mask = io_input_byte(data_port);
    io_output_byte(data_port, mask & (uint8_t) ~(1 << irq));
    __asm volatile("push %0; popf" : : "r"(flags));
}

void pic8259_irq_disable(uint8_t data_port, uint8_t irq)
{
    uint32_t flags;
    __asm volatile("pushf; pop %0; cli" : "=r"(flags));
    uint8_t mask = io_input_byte(data_port);
    io_output_byte(data_port, mask | (uint8_t)(1 << irq));
    __asm volatile("push %0; popf" : : "r"(flags));
}
