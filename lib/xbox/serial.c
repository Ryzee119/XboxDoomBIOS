// SPDX-License-Identifier: MIT
// Copyright (c) 2024 Ryzee119

#include "xbox.h"
#include <stdatomic.h>

static atomic_flag lock;

#define SERIAL_RECEIVE_BUFFER                0
#define SERIAL_RECEIVE_READY                 (1 << 5)
#define SERIAL_TRANSMIT_HOLDING              0
#define SERIAL_INTERRUPT_CONTROL             1
#define SERIAL_INTERRUPT_IDENTIFICATION      2
#define SERIAL_FIFO_CONTROL                  2
#define SERIAL_LINE_CONTROL                  3
#define SERIAL_LINE_CONTROL_8BIT             0x03
#define SERIAL_LINE_CONTROL_1STOP            0x00
#define SERIAL_LINE_CONTROL_NO_PARITY        0x00
#define SERIAL_LINE_CONTROL_DLAB             0x80
#define SERIAL_LINE_CONTROL_DLAB_DIVISER_LSB 0
#define SERIAL_LINE_CONTROL_DLAB_DIVISER_MSB 1

#define SERIAL_MODEM_CONTROL     4
#define SERIAL_MODEL_CONTROL_DTR 0x01
#define SERIAL_MODEL_CONTROL_RTS 0x02
#define SERIAL_LINE_STATUS       5
#define SERIAL_MODEM_STATUS      6

#define SIO_CONFIG_ENTER              0x55
#define SIO_CONFIG_EXIT               0xAA
#define SIO_CONFIG_SET_LOGICAL_DEVICE 0x07
#define SIO_CONFIG_ENABLE             0x30
#define SIO_CONFIG_BASE_ADDRESS_LOW   0x61
#define SIO_CONFIG_BASE_ADDRESS_HIGH  0x60

#define SIO_LOGICAL_DEVICE_COM1 0x04

void xbox_serial_init(void)
{
    io_output_byte(XBOX_SIO_CONFIG, SIO_CONFIG_ENTER);

    // Select COM1
    io_output_byte(XBOX_SIO_CONFIG, SIO_CONFIG_SET_LOGICAL_DEVICE);
    io_output_byte(XBOX_SIO_DATA, SIO_LOGICAL_DEVICE_COM1);

    // Enable COM1
    io_output_byte(XBOX_SIO_CONFIG, SIO_CONFIG_ENABLE);
    io_output_byte(XBOX_SIO_DATA, 1);

    // Set COM1 base address
    io_output_byte(XBOX_SIO_CONFIG, SIO_CONFIG_BASE_ADDRESS_HIGH);
    io_output_byte(XBOX_SIO_DATA, (XBOX_SERIAL_COM1 >> 8) & 0xFF);
    io_output_byte(XBOX_SIO_CONFIG, SIO_CONFIG_BASE_ADDRESS_LOW);
    io_output_byte(XBOX_SIO_DATA, (XBOX_SERIAL_COM1 >> 0) & 0xFF);

    io_output_byte(XBOX_SIO_CONFIG, SIO_CONFIG_EXIT);

    // Turn off interrupts
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL, 0);
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_INTERRUPT_CONTROL, 0);

    // Turn on dlab while we set the baud rate
    uint8_t lcr = io_input_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL);
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL, lcr | SERIAL_LINE_CONTROL_DLAB);

    // Set baud rate
    uint16_t divisor = 115200 / XBOX_SERIAL_BAUD;
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL_DLAB_DIVISER_MSB, (divisor >> 8) & 0xFF);
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL_DLAB_DIVISER_LSB, divisor & 0xFF);

    // Turn off dlab
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL, lcr);

    // Line control
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_CONTROL,
                   SERIAL_LINE_CONTROL_8BIT | SERIAL_LINE_CONTROL_1STOP | SERIAL_LINE_CONTROL_NO_PARITY);

    // Flow control
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_MODEM_CONTROL, SERIAL_MODEL_CONTROL_DTR | SERIAL_MODEL_CONTROL_RTS);
}

void xbox_serial_putchar(char character)
{
    spinlock_acquire(&lock);

    /* Wait for THRE (bit 5) to be high */
    while ((io_input_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_STATUS) & SERIAL_RECEIVE_READY) == 0) {
        system_yield(0);
    }
    io_output_byte(XBOX_SERIAL_COM1 + SERIAL_RECEIVE_BUFFER, character);

#if (0)
    if (character == '\n') {
        while ((io_input_byte(XBOX_SERIAL_COM1 + SERIAL_LINE_STATUS) & (1 << 5)) == 0) {
            system_yield(0);
        }
        io_output_byte(XBOX_SERIAL_COM1 + SERIAL_RECEIVE_BUFFER, '\r');
    }
#endif
    spinlock_release(&lock);
}
