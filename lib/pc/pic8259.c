// SPDX-License-Identifier: CC0-1.0

#include <stdatomic.h>
#include <stdint.h>

#include "cpu.h"
#include "io.h"
#include "lock.h"
#include "pic8259.h"

static atomic_flag lock;

void pic8259_irq_enable(uint8_t data_port, uint8_t irq)
{
    spinlock_acquire(&lock);
    uint8_t mask = io_input_byte(data_port);
    io_output_byte(data_port, mask & (uint8_t) ~(1 << irq));
    spinlock_release(&lock);
}

void pic8259_irq_disable(uint8_t data_port, uint8_t irq)
{
    spinlock_acquire(&lock);
    uint8_t mask = io_input_byte(data_port);
    io_output_byte(data_port, mask | (uint8_t)(1 << irq));
    spinlock_release(&lock);
}
