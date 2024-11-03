// SPDX-License-Identifier: CC0-1.0

#ifndef IA32_IO_H
#define IA32_IO_H

#include <stdint.h>

static inline __attribute__((always_inline)) void io_output_byte(uint16_t address, uint8_t value)
{
    __asm__ __volatile__("outb %b0,%w1" : : "a"(value), "Nd"(address));
}

static inline __attribute__((always_inline)) void io_output_word(uint16_t address, uint16_t value)
{
    __asm__ __volatile__("outw %0,%w1" : : "a"(value), "Nd"(address));
}

static inline __attribute__((always_inline)) void io_output_dword(uint16_t address, uint32_t value)
{
    __asm__ __volatile__("outl %0,%w1" : : "a"(value), "Nd"(address));
}

static inline __attribute__((always_inline)) uint8_t io_input_byte(uint16_t address)
{
    uint8_t v;
    __asm__ __volatile__("inb %w1,%0" : "=a"(v) : "Nd"(address));
    return v;
}

static inline __attribute__((always_inline)) uint16_t io_input_word(uint16_t address)
{
    uint16_t _v;
    __asm__ __volatile__("inw %w1,%0" : "=a"(_v) : "Nd"(address));
    return _v;
}

static inline __attribute__((always_inline)) uint32_t io_input_dword(uint16_t address)
{
    uint32_t _v;
    __asm__ __volatile__("inl %w1,%0" : "=a"(_v) : "Nd"(address));
    return _v;
}

static inline __attribute__((always_inline)) void mmio_output_byte(uint32_t address, uint8_t value)
{
    *(volatile uint8_t *)address = value;
}

static inline __attribute__((always_inline)) void mmio_output_word(uint32_t address, uint16_t value)
{
    *(volatile uint16_t *)address = value;
}

static inline __attribute__((always_inline)) void mmio_output_dword(uint32_t address, uint32_t value)
{
    *(volatile uint32_t *)address = value;
}

static inline __attribute__((always_inline)) uint8_t mmio_input_byte(uint32_t address)
{
    return *(volatile uint8_t *)address;
}

static inline __attribute__((always_inline)) uint32_t mmio_input_dword(uint32_t address)
{
    return *(volatile uint32_t *)address;
}

static inline __attribute__((always_inline)) uint16_t mmio_input_word(uint32_t address)
{
    return *(volatile uint16_t *)address;
}

#endif // IA32_IO_H
