#ifndef XBOX_H
#define XBOX_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pc/ata.h>
#include <pc/cpu.h>
#include <pc/dma8237.h>
#include <pc/ia32_compact.h>
#include <pc/ia32_msr.h>
#include <pc/idt.h>
#include <pc/io.h>
#include <pc/lock.h>
#include <pc/microcode.h>
#include <pc/pci_io.h>
#include <pc/pic8259.h>
#include <pc/pit8254.h>
#include <pc/smbus.h>

#include "audio.h"
#include "eeprom.h"
#include "encoder.h"
#include "led.h"
#include "pci.h"
#include "serial.h"
#include "smc.h"
#include "video.h"
#include "xtime.h"

#define XBOX_MIN(a, b)           ((a) < (b) ? (a) : (b))
#define XBOX_MAX(a, b)           ((a) > (b) ? (a) : (b))
#define XBOX_ARRAY_SIZE(a)       (sizeof(a) / sizeof((a)[0]))
#define XBOX_CLAMP(low, x, high) (XBOX_MIN(XBOX_MAX(x, low), high))

// See MTRRs in 2bmain.nasm
#define XBOX_GET_WRITE_COMBINE_PTR(ptr) ((void *)((uintptr_t)(ptr) | 0xF0000000))

#define XDEBUG
#ifdef XDEBUG
#define XPRINTF(...) printf(__VA_ARGS__)
#else
#define XPRINTF(...)
#endif

/* Main South Bridge Clocks */
// Measured, looks like its derived from the 27Mhz crystal on the motherboard (27 / 2)
// by clock generator IC ICS388R (C7C1) https://upload.wikimedia.org/wikipedia/commons/d/df/Xbox-Motherboard-Rev1.jpg
#define MCLCK 13500000

// 3.375Mhz (From KeQueryPerformanceFrequency)
#define ACPI_TIMER_FREQ         (MCLCK / 4)
#define XBOX_ACPI_TIMER_IO_PORT 0x8008 // Cromwell

// 1.125Mhz https://xboxdevwiki.net/Porting_an_Operating_System_to_the_Xbox_HOWTO#Timer_Frequency
#define PIC_TIMER_FREQ (MCLCK / 12)

/* Main North Bridge Clock */
#define NB_BASE_FEQ (16666666)                    // 16.6666666Mhz (From clock gen)
#define NB_FSB_FREQ (NB_BASE_FEQ * 8)             // 133.3333333Mhz (FSB) FIXME, multiplier can change with overclocking
#define NB_CPU_FREQ ((5500 * NB_FSB_FREQ) / 1000) // Yes 5.5x FSB

// MMIO Bases
#define XBOX_APIC_BASE   0xFEE00000
#define XBOX_IOAPIC_BASE 0xFEC00000

/* PCI Bus Address and MMIO - We match stock xbox */
#define PCI_XBOX_SYSTEM_BUS 0
#define PCI_XBOX_GPU_BUS    1

// Bus 0, device 0, function 0.
#define PCI_HOSTBRIDGE_DEVICE_ID   0
#define PCI_HOSTBRIDGE_FUNCTION_ID 0

// Bus 0, device 1, function 0.
#define PCI_LPCBRIDGE_DEVICE_ID          1
#define PCI_LPCBRIDGE_FUNCTION_ID        0
#define PCI_LPCBRIDGE_IO_REGISTER_BASE_0 0x8000

// Bus 0, device 1, function 1.
#define PCI_SMBUS_DEVICE_ID          1
#define PCI_SMBUS_FUNCTION_ID        1
#define PCI_SMBUS_IO_REGISTER_BASE_1 0xC000
#define PCI_SMBUS_IO_REGISTER_BASE_2 0xC200

// Bus 0, device 2, function 0.
#define PCI_USB0_DEVICE_ID   2
#define PCI_USB0_FUNCTION_ID 0

#define PCI_USB0_MEMORY_REGISTER_BASE_0 0xFED00000

// Bus 0, device 3, function 0.
#define PCI_USB1_DEVICE_ID              3
#define PCI_USB1_FUNCTION_ID            0
#define PCI_USB1_MEMORY_REGISTER_BASE_0 0xFED08000

// Bus 0, device 4, function 0.
#define PCI_NIC_DEVICE_ID              4
#define PCI_NIC_FUNCTION_ID            0
#define PCI_NIC_MEMORY_REGISTER_BASE_0 0xFEF00000
#define PCI_NIC_IO_REGISTER_BASE_1     0xE000

// Bus 0, device 5, function 0.
#define PCI_APU_DEVICE_ID              5
#define PCI_APU_FUNCTION_ID            0
#define PCI_APU_MEMORY_REGISTER_BASE_0 0xFE800000

// Bus 0, device 6, function 0.
#define PCI_ACI_DEVICE_ID              6
#define PCI_ACI_FUNCTION_ID            0
#define PCI_ACI_IO_REGISTER_BASE_0     0xD000
#define PCI_ACI_IO_REGISTER_BASE_1     0xD200
#define PCI_ACI_MEMORY_REGISTER_BASE_2 0xFEC00000

// Bus 0, device 9, function 0.
#define PCI_IDE_DEVICE_ID          9
#define PCI_IDE_FUNCTION_ID        0
#define PCI_IDE_IO_REGISTER_BASE_4 0xFF60

// Bus 0, device 30, function 0.
#define PCI_AGPBRIDGE_DEVICE_ID   30
#define PCI_AGPBRIDGE_FUNCTION_ID 0

// Bus 1, device 0, device 0.
#define PCI_GPU_DEVICE_ID              0
#define PCI_GPU_FUNCTION_ID            0
#define PCI_GPU_MEMORY_REGISTER_BASE_0 0xFD000000

#if (0)
#define A20_GATE_SPEAKER_BASE  0x0060
#define CMOS_RTC_BASE          0x0070
#define DMA_PAGE_ADDRESS_BASE  0x0080
#define SLAVE_PIC_BASE         0x00A0
#define DMA_CHANNELS_4_7_BASE  0x00C0
#define FPU_ERROR_CONTROL_BASE 0x00F0
#define IDE_BASE               0x01F0
#define PCI_CONFIG_BASE        0x0CF8
#define SMBUS_I2C_BASE         0x1000
#endif

// PIC Interrupt Controller
// What retail uses - we also move away from FreeRTOS which uses 0x20 and 0x21
#define XBOX_PIC1_BASE_VECTOR_ADDRESS 0x30
#define XBOX_PIC2_BASE_VECTOR_ADDRESS 0x38
#define XBOX_PIC_IRQ_TO_VECTOR(irq_num)                                                                                \
    (irq_num < 8 ? (XBOX_PIC1_BASE_VECTOR_ADDRESS + irq_num) : (XBOX_PIC2_BASE_VECTOR_ADDRESS + (irq_num - 8)))

#define XBOX_PIC1_COMMAND_PORT 0x20
#define XBOX_PIC1_DATA_PORT    (XBOX_PIC1_COMMAND_PORT + 1)
#define XBOX_PIC2_COMMAND_PORT 0xA0
#define XBOX_PIC2_DATA_PORT    (XBOX_PIC2_COMMAND_PORT + 1)

#define XBOX_PIT_TIMER_IRQ 0
#define XBOX_PIC_USB0_IRQ  1
#define XBOX_PIC_SLAVE_IRQ 2 // PIC2 is connected to IRQ2 of PIC1
#define XBOX_PIC_GPU_IRQ   3
#define XBOX_PIC_NIC_IRQ   4
#define XBOX_PIC_APU_IRQ   5
#define XBOX_PIC_ACI_IRQ   6
#define XBOX_PIC_USB1_IRQ  9
#define XBOX_PIC_SMC_IRQ   12
#define XBOX_PIC_IDE_IRQ   14

// DMA
#define XBOX_DMA_CHANNELS_0_3_PORT 0x00
#define XBOX_DMA_CHANNELS_4_7_PORT 0xC0

// PIT
#define XBOX_PIT_CHANNEL0     0x40
#define XBOX_PIT_COMMAND_PORT 0x43

// ACPI
#define XBOX_ACPI_TIMER_PORT 0x8008 // Cromwell

// SuperIO
#define XBOX_SIO_CONFIG  0x2E
#define XBOX_SIO_DATA    0x2F
#define XBOX_SIO_INDEX   0x2F
#define XBOX_SERIAL_COM1 0x03F8
#define XBOX_SERIAL_BAUD 115200

// SMBus
#define XBOX_SMBUS_ADDRESS_SMC              0x20
#define XBOX_SMBUS_ADDRESS_EEPROM           0xA8
#define XBOX_SMBUS_ADDRESS_TEMP             0x98
#define XBOX_SMBUS_ADDRESS_ENCODER_CONEXANT 0x8A
#define XBOX_SMBUS_ADDRESS_ENCODER_FOCUS    0xD4
#define XBOX_SMBUS_ADDRESS_ENCODER_XCALIBUR 0xE0

// IDE ATA
#define XBOX_ATA_PRIMARY_BUS_IO_BASE   0x1F0
#define XBOX_ATA_PRIMARY_BUS_CTRL_BASE 0x3F6
#define XBOX_ATA_BUSMASTER_BASE        PCI_IDE_IO_REGISTER_BASE_4

void xbox_interrupt_enable(uint8_t irq, uint8_t enable);

#endif