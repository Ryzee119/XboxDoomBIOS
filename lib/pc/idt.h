// SPDX-License-Identifier: CC0-1.0

#ifndef IA32_IDT_H
#define IA32_IDT_H

#include <stdint.h>

#define IDT_PRESENT        0x80
#define IDT_TYPE_TASK_GATE (IDT_PRESENT | 0x5)
#define IDT_TYPE_INT_GATE  (IDT_PRESENT | 0xE)
#define IDT_TYPE_TRAP_GATE (IDT_PRESENT | 0xF)

struct IDTEntry
{
    uint16_t usISRLow;
    uint16_t usSegmentSelector;
    uint8_t ucZero;
    uint8_t ucFlags;
    uint16_t usISRHigh;
} __attribute__((packed));
typedef struct IDTEntry IDTEntry_t;

struct IDTPointer
{
    uint16_t usTableLimit;
    uint32_t ulTableBase;
} __attribute__((__packed__));
typedef struct IDTPointer IDTPointer_t;

void idt_install_interrupt(uint8_t vector, void (*handler)(void), uint8_t type);

#endif