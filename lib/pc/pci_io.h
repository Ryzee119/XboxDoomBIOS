// SPDX-License-Identifier: CC0-1.0

#ifndef PCI_IO_H
#define PCI_IO_H

#include "io.h"
#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

#define PCI_COMMAND_IO          0x1   /* Enable response in I/O space */
#define PCI_COMMAND_MEMORY      0x2   /* Enable response in Memory space */
#define PCI_COMMAND_MASTER      0x4   /* Enable bus mastering */
#define PCI_COMMAND_SPECIAL     0x8   /* Enable response to special cycles */
#define PCI_COMMAND_INVALIDATE  0x10  /* Use memory write and invalidate */
#define PCI_COMMAND_VGA_PALETTE 0x20  /* Enable palette snooping */
#define PCI_COMMAND_PARITY      0x40  /* Enable parity checking */
#define PCI_COMMAND_WAIT        0x80  /* Enable address/data stepping */
#define PCI_COMMAND_SERR        0x100 /* Enable SERR */
#define PCI_COMMAND_FAST_BACK   0x200 /* Enable back-to-back writes */

// https://wiki.osdev.org/PCI
typedef struct
{
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    union
    {
        struct
        {
            uint32_t base_address[6];
            uint32_t cardbus_cis_pointer;
            uint16_t subsystem_vendor_id;
            uint16_t subsystem_id;
            uint32_t expansion_rom_base_address;
            uint8_t capabilities_pointer;
            uint8_t reserved[7];
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint8_t min_grant;
            uint8_t max_latency;
        } type_0;
        struct
        {
            uint32_t base_address[2];
            uint8_t primary_bus_number;
            uint8_t secondary_bus_number;
            uint8_t subordinate_bus_number;
            uint8_t secondary_latency_timer;
            uint8_t io_base;
            uint8_t io_limit;
            uint16_t secondary_status;
            uint16_t memory_base;
            uint16_t memory_limit;
            uint16_t prefetchable_memory_base;
            uint16_t prefetchable_memory_limit;
            uint32_t prefetchable_base_upper32;
            uint32_t prefetchable_limit_upper32;
            uint16_t io_base_upper16;
            uint16_t io_limit_upper16;
            uint8_t capabilities_pointer;
            uint8_t reserved[3];
            uint32_t rom_address;
            uint8_t interrupt_line;
            uint8_t interrupt_pin;
            uint16_t bridge_control;
        } type_1;
    };
    // Type 2...
} pci_header_t;

uint8_t pci_io_input_byte(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
uint16_t pci_io_input_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
uint32_t pci_io_input_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg);
void pci_io_output_byte(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint8_t val);
void pci_io_output_word(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint16_t val);
void pci_io_output_dword(uint8_t bus, uint8_t dev, uint8_t func, uint8_t reg, uint32_t val);
void pci_io_output_n(uint8_t bus, uint8_t dev, uint8_t func, uint32_t size_of_data, void *data);
void pci_io_input_n(uint8_t bus, uint8_t dev, uint8_t func, uint32_t size_of_data, void *data);

#endif